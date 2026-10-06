#pragma once
#include <JuceHeader.h>

// G-AcidBase update check.
//
// The plugin makes one anonymous GET - docs/gacidbase/version.json on the
// product site - and compares the "latest" in it to the version it was built
// as. Everything that decides what the result dialog says lives here as pure
// functions, so tests/Features.cpp can check the wording, the links and the
// version comparison without a network or a window; the fetch thread and the
// manual button is in the editor; the automatic check belongs to the processor
// session, so closing and reopening its editor never repeats the request.
//
// The website generates the feed from its changelog and release metadata, with the same
// shape as GoaSynth's:
//   { "latest": "1.1.0", "url": "...", "notes": ["..."], "released": "...",
//     "installer_size": "4.5 MB" }
// Only "latest" is required. The rest are optional and an older feed simply
// leaves the matching line out of the dialog.
namespace gacid
{
// The running release version, from CMake's project(). Tests that compile this
// without CMake fall back to the same string project() currently names - keep
// the two in sync when bumping, and bump docs/gacidbase/version.json with them.
#ifndef GACIDBASE_VERSION
 #define GACIDBASE_VERSION "1.1.1"
#endif

inline juce::String versionString() { return juce::String (GACIDBASE_VERSION); }

// The product site, the feed it publishes, and where a release hangs its
// install. Same origin for the first two, one path apart; the install lives on
// GitHub because that is where the release assets are attached.
inline constexpr const char* siteUrl = "https://dakagoa.github.io/GoaSynth/gacidbase/";
inline constexpr const char* updateFeedUrl = "https://dakagoa.github.io/GoaSynth/gacidbase/version.json";
inline constexpr const char* releaseDownloadBase = "https://github.com/DakaGoa/G-AcidBase/releases/download";
inline const juce::String changeLogUrl = juce::String(siteUrl) + "#changelog";

// The asset a release attaches and the dialog downloads: the Setup installer,
// which puts the plugin and the standalone where the DAW looks for them, rather
// than a folder somebody has to copy by hand. The name carries the version -
// GitHub asset URLs are per-tag, and a versioned file cannot be mistaken for
// another build's installer. Named here rather than in the feed so a mistyped
// feed cannot send a click to a file that was never uploaded.
inline juce::String installerAssetName (const juce::String& version)
{
    return "G-AcidBase-Setup-" + version + ".exe";
}

inline juce::String installerUrl (const juce::String& version)
{
    return juce::String (releaseDownloadBase) + "/v" + version + "/" + installerAssetName (version);
}

inline juce::String releaseNotesUrl (const juce::String& version)
{
    return juce::String (siteUrl) + "#v" + version;
}

// Compares versions as "major.minor.patch": <0 when a is older than b, 0 when
// equal, >0 when newer. Every field counts - a 1.1.1 fix release has to be
// offered to someone running 1.1.0 - and non-numeric tokens count as 0, so a
// malformed feed can never outrank this build. Pure, so tests drive it the way
// the check does.
inline int compareVersions (const juce::String& a, const juce::String& b)
{
    const auto part = [] (const juce::String& s, int index)
    {
        const auto tokens = juce::StringArray::fromTokens (s, ".", {});
        return index < tokens.size() ? tokens[index].getIntValue() : 0;
    };
    for (int index = 0; index < 3; ++index)
        if (part (a, index) != part (b, index))
            return part (a, index) < part (b, index) ? -1 : 1;
    return 0;
}

// Pulls the "notes" array out of a parsed feed - the release notes the dialog
// shows under "What's new". Plain, non-empty strings only, capped: the wire is
// untrusted, and the card has room for a handful of lines (the release page has
// the full list).
inline juce::StringArray latestReleaseNotes (const juce::var& feed, int maxNotes = 4)
{
    juce::StringArray out;
    if (const auto* notes = feed.getProperty ("notes", {}).getArray())
        for (const auto& note : *notes)
        {
            const auto line = note.toString().trim();
            if (line.isNotEmpty())
                out.add (line);
            if (out.size() >= maxNotes)
                break;
        }
    return out;
}

// What the feed said, in the terms the dialog needs. "reachable" is false both
// when the fetch failed and when it succeeded with a feed that names no
// numeric version - an error page, a CDN challenge or a truncated file is
// "cannot check", never "up to date".
struct UpdateOutcome
{
    bool reachable = false;
    bool newer = false;
    juce::String latestVersion, released, packageSize;
    juce::StringArray notes;
};

inline UpdateOutcome readFeed (const juce::var& feed, const juce::String& runningVersion)
{
    UpdateOutcome outcome;
    const auto latest = feed.getProperty ("latest", {}).toString().trim();
    outcome.reachable = latest.isNotEmpty() && latest.containsOnly ("0123456789.");
    if (! outcome.reachable)
        return outcome;

    outcome.latestVersion = latest;
    outcome.released = feed.getProperty ("released", {}).toString().trim();
    outcome.packageSize = feed.getProperty ("installer_size", {}).toString().trim();
    outcome.notes = latestReleaseNotes (feed);
    outcome.newer = compareVersions (runningVersion, latest) < 0;
    return outcome;
}

// The dialog's whole content, decided without any UI: title, body, the row that
// downloads (empty when there is nothing to download), the second row that
// opens the release notes (empty when there is no reason to send anyone to the
// site), and the text that names the download when the address behind it is too
// long to print. Tests assert on these strings; the overlay only lays them out.
struct UpdateMessage
{
    juce::String title, message, downloadUrl, notesUrl, downloadLabel;
};

inline UpdateMessage describe (const UpdateOutcome& outcome, const juce::String& runningVersion)
{
    UpdateMessage result;

    if (! outcome.reachable)
    {
        // Say so rather than say nothing: a check that fails silently is
        // indistinguishable from a dead menu item. The one link offered points
        // at the site, which is where a person finds the download by hand.
        result.title = "G-ACIDBASE";
        result.message = "Could not reach the update feed.\n\n"
                         "Latest version and downloads:";
        result.downloadUrl = siteUrl;
        return result;
    }

    if (outcome.newer)
    {
        result.title = "UPDATE AVAILABLE";
        result.message = "G-AcidBase " + outcome.latestVersion + " is available (you are running "
                         + runningVersion + ").";

        // When it shipped. Optional: an older feed without the field omits it.
        if (outcome.released.isNotEmpty())
            result.message << "\nReleased " << outcome.released << ".";

        // What changed. Optional the same way, capped by latestReleaseNotes.
        if (outcome.notes.size() > 0)
        {
            result.message << "\n\nWhat's new:";
            for (const auto& note : outcome.notes)
                result.message << "\n\u2022 " << note;
        }

        // The size answers "how big is this?" in the same row that asks for the
        // click. Optional like the two lines above.
        result.message << "\n\n"
                       << (outcome.packageSize.isNotEmpty()
                               ? "Download the new installer (" + outcome.packageSize + "):"
                               : juce::String ("Download the new installer:"));

        // The download row names the file rather than the address: the GitHub
        // URL is far too long for the row, so it stays the link's target.
        result.downloadUrl = installerUrl (outcome.latestVersion);
        result.notesUrl = releaseNotesUrl (outcome.latestVersion);
        result.downloadLabel = installerAssetName (outcome.latestVersion);
        return result;
    }

    // Up to date: nothing to download, but the release notes are one click away.
    result.title = "G-ACIDBASE";
    result.message = "You are running the latest version (" + runningVersion + ").";
    result.notesUrl = releaseNotesUrl (runningVersion);
    return result;
}

// The fetch. One GET of the feed, on its own thread, that hands the outcome
// back exactly once - success or failure - through the callback. Every path
// through run() ends in that callback: a check that fails silently is
// indistinguishable from a dead button. The URL is a parameter rather than a
// constant so tests can point the real fetch at a local server.
class UpdateCheckThread : public juce::Thread
{
public:
    UpdateCheckThread (const juce::String& feedUrl, std::function<void (const UpdateOutcome&)> done)
        : juce::Thread ("gacidbase update check"), url (feedUrl), finished (std::move (done)) {}

    ~UpdateCheckThread() override { stopThread(10000); }

    void run() override
    {
        UpdateOutcome outcome;
        int statusCode = 0;
        // A plain GET: this reads a static file, and GitHub Pages answers a
        // bodyless POST with 405. inAddress keeps the empty parameter list in
        // the URL rather than sending it as a body.
        const auto stream = juce::URL (url).createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (8000).withStatusCode (&statusCode));
        if (stream != nullptr && statusCode >= 200 && statusCode < 300)
            outcome = readFeed (juce::JSON::parse (stream->readEntireStreamAsString()), versionString());
        finished (outcome);
    }

    juce::String url;
    std::function<void (const UpdateOutcome&)> finished;
};

// One automatic attempt per processor instance, started on its first editor
// opening. No state is saved in presets, no network runs on the audio thread,
// and only a reachable/newer result is eligible for a single notification.
// The worker owns no editor pointer: a result can wait while the window is shut.
class SessionUpdateCheck
{
public:
    explicit SessionUpdateCheck(const juce::String& url = updateFeedUrl)
        : worker(url, [this](const UpdateOutcome& result)
          {
              const juce::ScopedLock guard(lock);
              outcome=result;
              completed=true;
          }) {}

    ~SessionUpdateCheck() { worker.stopThread(10000); }

    bool startOnce()
    {
        const juce::ScopedLock guard(lock);
        if(started)return false;
        started=true; // an offline/failed attempt counts too: no hidden retries
        if(!worker.startThread())completed=true;
        return true;
    }

    const juce::String& feedUrl() const { return worker.url; }

    bool isComplete() const
    {
        const juce::ScopedLock guard(lock);
        return completed;
    }

    bool takeNotification(UpdateOutcome& result)
    {
        const juce::ScopedLock guard(lock);
        if(!completed||notified||!outcome.reachable||!outcome.newer)return false;
        notified=true;
        result=outcome;
        return true;
    }

    void acknowledgeNotification()
    {
        const juce::ScopedLock guard(lock);
        notified=true; // a manual newer-version result also counts as notified
    }

private:
    mutable juce::CriticalSection lock;
    UpdateOutcome outcome;
    bool started=false,completed=false,notified=false;
    UpdateCheckThread worker;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionUpdateCheck)
};

// The result card. A child of the editor's surface rather than a native box
// because a native box cannot host a link at all, and the whole point of the
// two download outcomes is that the address is clickable - plus a native box
// was free to land behind the arrange view, which reads as "nothing happens".
struct UpdateResultOverlay : juce::Component
{
    UpdateResultOverlay()
    {
        addAndMakeVisible (titleLabel);
        addAndMakeVisible (messageLabel);
        addAndMakeVisible (downloadLink);
        addAndMakeVisible (notesLink);
        addAndMakeVisible (okButton);
        addAndMakeVisible (closeButton);

        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb8ff38));
        messageLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe8eef7));
        for (auto* link : { &downloadLink, &notesLink })
        {
            link->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff42e8da));
            link->setJustificationType (juce::Justification::centredLeft);
        }
        okButton.onClick = [this] { setVisible (false); };
        closeButton.onClick = [this] { setVisible (false); };

        // Clicking the darkened backdrop outside the card dismisses it too.
        mouseDownCallback = [this] (const juce::MouseEvent&) { setVisible (false); };
    }

    void configure (const juce::String& title, const juce::String& message,
                    const juce::String& downloadUrl,
                    const juce::String& notesUrl = {},
                    const juce::String& downloadLabel = {})
    {
        titleLabel.setText (title, juce::dontSendNotification);
        messageLabel.setText (message, juce::dontSendNotification);

        // An empty address hides the row: "up to date" has nothing to download,
        // and the unreachable branch sends nobody to a site the check just
        // failed to reach twice.
        downloadLink.setURL (juce::URL (downloadUrl));
        downloadLink.setButtonText (downloadUrl.isNotEmpty()
                                        ? (downloadLabel.isNotEmpty() ? downloadLabel : downloadUrl)
                                        : juce::String());
        downloadLink.setTooltip (downloadUrl);
        downloadLink.setVisible (downloadUrl.isNotEmpty());

        notesLink.setURL (juce::URL (notesUrl));
        notesLink.setButtonText (notesUrl.isNotEmpty() ? "View release notes" : juce::String());
        notesLink.setTooltip (notesUrl);
        notesLink.setVisible (notesUrl.isNotEmpty());

        resized();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (mouseDownCallback && ! cardBounds().contains (e.getPosition()))
            mouseDownCallback (e);
    }

    // The centred card every row is laid out against.
    juce::Rectangle<int> cardBounds() const noexcept
    {
        return getLocalBounds().withSizeKeepingCentre (520, 360);
    }

    void resized() override
    {
        auto area = cardBounds().reduced (24);
        titleLabel.setBounds (area.removeFromTop (26));
        area.removeFromTop (8);
        messageLabel.setBounds (area.removeFromTop (150));
        area.removeFromTop (10);
        downloadLink.setBounds (area.removeFromTop (24));
        notesLink.setBounds (area.removeFromTop (24));
        area.removeFromTop (12);
        auto buttons = area.removeFromTop (28);
        okButton.setBounds (buttons.removeFromRight (120));
        buttons.removeFromRight (10);
        closeButton.setBounds (buttons.removeFromLeft (120));
    }

    void paint (juce::Graphics& g) override
    {
        // The backdrop, then the card. The darkening is what makes it read as a
        // dialog without taking the window away from the host.
        g.fillAll (juce::Colours::black.withAlpha (0.62f));
        auto card = cardBounds().toFloat();
        g.setColour (juce::Colour (0xff141d29));
        g.fillRoundedRectangle (card, 12.0f);
        g.setColour (juce::Colour (0xff283749));
        g.drawRoundedRectangle (card.reduced (0.5f), 12.0f, 1.0f);
    }

    juce::Label titleLabel, messageLabel;
    juce::HyperlinkButton downloadLink, notesLink;
    juce::TextButton okButton { "OK", "Dismiss the update result" };
    juce::TextButton closeButton { "\u00d7", "Back to the synth" };

private:
    std::function<void (const juce::MouseEvent&)> mouseDownCallback;
};
}
