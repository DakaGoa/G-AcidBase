#!/usr/bin/env python3
"""Publish a G-AcidBase release: the GitHub release and the assets on its row.

    python scripts/publish-release.py --tag v1.1.1 \
        --notes-from-changelog ../goasynth-site/tools/gacidbase/CHANGELOG.md
    python scripts/publish-release.py --verify

`package.py` builds and gates the files; this uploads them, under the names the
product page and the update dialog link:

    G-AcidBase-Windows-x64.zip              the package (installer + manual copy)
    G-AcidBase-Setup-<version>.exe          the installer on its own
    G-AcidBase-Setup-<version>.exe.sha256   its one-line checksum
    SHA256SUMS.txt                          what the ZIP is verified against

Re-running is safe: an existing release is reused and an asset of the same name
is replaced, so a run interrupted by a failed upload can simply be repeated.

`--verify` is the release's last gate. Nothing here can know whether the site
names this version - the feed is refreshed from the release, and the release is
built from the package - so the check that the feed, the download row and every
address on it agree is the one that has to run afterwards, against the live
site. It exits non-zero until they do.

The token comes from git's credential store (`git credential fill`), so nothing
is typed here and nothing is written to disk. Plain Python 3, no dependencies.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
VERSION_RE = r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)"
OWNER_REPO = "DakaGoa/G-AcidBase"
API = f"https://api.github.com/repos/{OWNER_REPO}"
UPLOADS = f"https://uploads.github.com/repos/{OWNER_REPO}"
FEED_URL = "https://dakagoa.github.io/GoaSynth/gacidbase/version.json"
SITE_URL = "https://dakagoa.github.io/GoaSynth/gacidbase/"
ZIP_NAME = "G-AcidBase-Windows-x64.zip"


def die(message: str) -> None:
    print("publish-release: " + message, file=sys.stderr)
    sys.exit(1)


def note(message: str) -> None:
    print("publish-release: " + message)


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True,
                          text=True, check=True).stdout.strip()


def project_version() -> str:
    text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"project\(GAcidBase VERSION (" + VERSION_RE + ")", text)
    if not match:
        die("could not read the version from project() in CMakeLists.txt")
    return match.group(1)


# Set by --token-from-env: CI has no git credential store to ask.
TOKEN_FROM_ENV = False


def github_token() -> str:
    """The token git already stores for github.com - never printed."""
    if TOKEN_FROM_ENV:
        token = os.environ.get("GITHUB_TOKEN", "").strip()
        if token:
            return token
        die("GITHUB_TOKEN is empty")
    try:
        result = subprocess.run(["git", "credential", "fill"], cwd=ROOT,
                                capture_output=True, text=True, check=True,
                                input="protocol=https\nhost=github.com\n\n")
    except (OSError, subprocess.CalledProcessError) as error:
        die(f"could not read git's stored GitHub credential ({error})")
    for line in result.stdout.splitlines():
        if line.startswith("password="):
            token = line[len("password="):].strip()
            if token:
                return token
    die("git holds no GitHub credential - run: git credential approve "
        "(protocol=https, host=github.com, username=<user>, password=<token>)")


def request(url: str, token: str, data: bytes | None = None, method: str = "GET",
            content_type: str = "application/vnd.github+json", timeout: int = 60):
    headers = {"Accept": "application/vnd.github+json",
               "Authorization": f"Bearer {token}",
               "User-Agent": "gacidbase-release-publisher",
               "X-GitHub-Api-Version": "2022-11-28"}
    if data is not None:
        headers["Content-Type"] = content_type
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    return urllib.request.urlopen(req, timeout=timeout)


def fetch_json(url: str, token: str) -> dict:
    with request(url, token) as response:
        return json.load(response)


def section_body(changelog_path: Path, version: str) -> str:
    """The release body: that version's changelog section, verbatim markdown."""
    if not changelog_path.is_file():
        die(f"{changelog_path} does not exist")
    lines = changelog_path.read_text(encoding="utf-8").splitlines()
    heading = re.compile(r"^## \[" + re.escape(version) + r"\] - \d{4}-\d{2}-\d{2}\s*$")
    start = None
    for index, line in enumerate(lines):
        if heading.match(line):
            start = index
            break
        if line.startswith("## "):
            continue
    if start is None:
        die(f"{changelog_path} has no '## [{version}] - YYYY-MM-DD' section to publish")
    end = len(lines)
    for index in range(start + 1, len(lines)):
        if lines[index].startswith("## "):
            end = index
            break
    body = "\n".join(lines[start:end]).strip()
    if not body:
        die(f"{changelog_path}'s {version} section is empty")
    return body


def release_assets(version: str) -> dict[str, Path]:
    """The four files the release carries, keyed by the name they upload under."""
    setup_name = f"G-AcidBase-Setup-{version}.exe"
    wanted = {
        ZIP_NAME: DIST / ZIP_NAME,
        setup_name: DIST / setup_name,
        setup_name + ".sha256": DIST / (setup_name + ".sha256"),
        "SHA256SUMS.txt": DIST / "SHA256SUMS.txt",
    }
    for name, path in wanted.items():
        if not path.is_file():
            die(f"missing {path} - run python scripts/package.py first")
    return wanted


def guard_tree() -> str:
    """A release is built from a commit, and that commit must be on the remote."""
    dirty = [line for line in git("status", "--porcelain").splitlines() if line[:2] != "??"]
    if dirty:
        print("publish-release: these tracked files have uncommitted changes:")
        for line in dirty:
            print("   " + line)
        die("commit them first - a release is built from a commit")
    head = git("rev-parse", "HEAD")
    remote = subprocess.run(["git", "ls-remote", "origin", "refs/heads/main"],
                            cwd=ROOT, capture_output=True, text=True, check=True)
    remote_head = remote.stdout.split()[0] if remote.stdout.split() else ""
    if remote_head != head:
        die(f"HEAD {head[:10]} is not origin/main ({remote_head[:10] or 'unknown'}) - push first")
    return head


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def create_or_reuse_release(token: str, tag: str, version: str, body: str, head: str) -> dict:
    payload = {"tag_name": tag, "name": f"G-AcidBase {version}", "body": body,
               "draft": False, "prerelease": False, "target_commitish": head}
    try:
        with request(f"{API}/releases", token, json.dumps(payload).encode(), "POST") as response:
            release = json.load(response)
        note(f"created {release['html_url']}")
        return release
    except urllib.error.HTTPError as error:
        if error.code != 422:
            die(f"creating the release failed: HTTP {error.code} {error.read().decode(errors='replace')[:300]}")
    # 422: this tag already has a release (a repeated run, or an earlier failed
    # upload). Reuse it so the uploads below can finish the job.
    release = fetch_json(f"{API}/releases/tags/{tag}", token)
    note(f"reusing the existing release {release['html_url']}")
    if release.get("body") != body:
        with request(f"{API}/releases/{release['id']}", token,
                     json.dumps({"body": body}).encode(), "PATCH") as response:
            release = json.load(response)
        note("replaced the release notes with the changelog section for this version")
    return release


def upload(token: str, release: dict, name: str, path: Path) -> str:
    for asset in release.get("assets", []):
        if asset["name"] == name:
            request(f"{API}/releases/assets/{asset['id']}", token, method="DELETE")
            note(f"removed the previous {name}")
    with request(f"{UPLOADS}/releases/{release['id']}/assets?name={name}", token,
                 path.read_bytes(), "POST", "application/octet-stream",
                 timeout=600) as response:
        uploaded = json.load(response)
    note(f"uploaded {name} ({path.stat().st_size:,} bytes)")
    return uploaded["browser_download_url"]


def publish(args) -> int:
    version = project_version()
    tag = args.tag or f"v{version}"
    if not re.fullmatch("v" + VERSION_RE, tag):
        die("--tag must be vMAJOR.MINOR.PATCH")
    if tag != f"v{version}":
        die(f"--tag {tag} is not this build: CMakeLists.txt says {version}")
    if args.notes and (args.notes_file or args.notes_from_changelog):
        die("pass one of --notes, --notes-file or --notes-from-changelog")
    if args.notes_from_changelog:
        body = section_body(Path(args.notes_from_changelog), version)
    elif args.notes_file:
        path = Path(args.notes_file)
        if not path.is_file():
            die(f"{path} does not exist")
        body = path.read_text(encoding="utf-8").strip()
        if not body:
            die(f"{path} is empty")
    elif args.notes:
        body = args.notes
    else:
        die("no release notes: pass --notes-from-changelog, --notes-file or --notes")

    assets = release_assets(version)
    head = guard_tree()
    token = github_token()

    note(f"publishing {tag} from {head[:10]}")
    release = create_or_reuse_release(token, tag, version, body, head)
    for name, path in assets.items():
        upload(token, release, name, path)
    note(f"release: {release['html_url']}")
    note("next: add the changelog section to the site, refresh the feed "
         "(python tools/make-gacidbase-feed.py --refresh --tag " + tag + "), "
         "then run --verify")
    return 0


def verify(args) -> int:
    """Nothing local can prove the site agrees: ask the live site."""
    version = project_version()
    token = github_token()
    problems: list[str] = []

    try:
        with urllib.request.urlopen(FEED_URL, timeout=20) as response:
            feed = json.load(response)
    except Exception as error:
        die(f"could not read {FEED_URL} ({error})")
    if feed.get("latest") != version:
        problems.append(f"the feed says latest={feed.get('latest')!r}, this build is {version} - "
                        "run tools/make-gacidbase-feed.py --refresh in the site repository")
    if feed.get("url") != SITE_URL:
        problems.append(f"the feed's url is {feed.get('url')!r}")
    if not feed.get("installer_size"):
        problems.append("the feed carries no installer_size")

    release = fetch_json(f"{API}/releases/tags/v{version}", token)
    published = {asset["name"]: asset for asset in release.get("assets", [])}
    assets = release_assets(version)
    for name, path in assets.items():
        asset = published.get(name)
        if asset is None:
            problems.append(f"{name} is not attached to the release")
            continue
        if asset["size"] != path.stat().st_size:
            problems.append(f"{name} on the release is {asset['size']:,} bytes, "
                            f"the packaged file is {path.stat().st_size:,}")
        try:
            with urllib.request.urlopen(asset["browser_download_url"], timeout=30) as response:
                head_bytes = response.read(1)
            if not head_bytes:
                problems.append(f"{name} downloads nothing")
        except Exception as error:
            problems.append(f"{name} does not download ({error})")

    # The installer's published checksum must be the one this machine computed,
    # so the bytes buyers verify are the bytes that were packaged.
    setup_name = f"G-AcidBase-Setup-{version}.exe"
    checksum_asset = published.get(setup_name + ".sha256")
    if checksum_asset is not None:
        with urllib.request.urlopen(checksum_asset["browser_download_url"], timeout=30) as response:
            quoted = response.read().decode("utf-8", "replace").split()[0].lower()
        local = sha256_file(DIST / setup_name)
        if quoted != local:
            problems.append(f"the published {setup_name}.sha256 ({quoted[:16]}...) does not match "
                            f"the packaged installer ({local[:16]}...)")

    if feed.get("latest") == version:
        for label, url in (("the installer", f"https://github.com/{OWNER_REPO}/releases/download/"
                                             f"v{version}/{setup_name}"),
                           ("the ZIP", f"https://github.com/{OWNER_REPO}/releases/download/"
                                       f"v{version}/{ZIP_NAME}"),
                           ("the checksums", f"https://github.com/{OWNER_REPO}/releases/download/"
                                             f"v{version}/SHA256SUMS.txt"),
                           ("the release page", release["html_url"]),
                           ("the product page", SITE_URL)):
            try:
                with urllib.request.urlopen(url, timeout=30) as response:
                    if response.status != 200:
                        problems.append(f"{label} answered HTTP {response.status}: {url}")
            except Exception as error:
                problems.append(f"{label} is unreachable ({error}): {url}")

    for problem in problems:
        print("publish-release: " + problem, file=sys.stderr)
    if problems:
        return 1
    print(f"publish-release: {tag_text(version)} is published and the site names it: "
          f"feed, installer, ZIP, checksums and both pages answer")
    return 0


def tag_text(version: str) -> str:
    return f"v{version}"


def main() -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--tag", help=f"the release tag (default: v{project_version()})")
    parser.add_argument("--notes-from-changelog", metavar="CHANGELOG.md",
                        help="publish that version's section of this changelog as the release body")
    parser.add_argument("--notes-file", metavar="FILE", help="markdown body for the release")
    parser.add_argument("--notes", metavar="TEXT", help="release body on the command line")
    parser.add_argument("--verify", action="store_true",
                        help="check the live feed, the release assets and the row's addresses")
    parser.add_argument("--token-from-env", action="store_true",
                        help="read the token from GITHUB_TOKEN instead of git's credential store")
    args = parser.parse_args()
    global TOKEN_FROM_ENV
    TOKEN_FROM_ENV = args.token_from_env
    try:
        return verify(args) if args.verify else publish(args)
    except urllib.error.HTTPError as error:
        die(f"GitHub answered HTTP {error.code}: {error.read().decode(errors='replace')[:300]}")
    except (OSError, ValueError, KeyError) as error:
        die(str(error))
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
