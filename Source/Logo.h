#pragma once
#include <JuceHeader.h>

// G-AcidBase vector logo.
//
// Geometry is normalised to a 0..1 square and mirrors scripts/make_logo.py, which
// writes assets/G-AcidBase-Logo.svg and the PNG rasters from the same numbers.
// Keep both files in step: tests/Features.cpp compares this drawing against the
// packaged 512 px raster, so any change here must be regenerated with
// `python scripts/make_logo.py`.
//
// The animated overload below is deliberately additive: it draws the same mark and
// then layers motion on top, so the static image stays a faithful copy of the shipped
// artwork (phase 0 / resonance 0 is a visual no-op).
namespace GAcidLogo
{
struct AnimationState
{
    float phase = 0.f;      ///< Position in a one-beat cycle, 0 on the beat, driven by the host tempo.
    float resonance = 0.f;  ///< Normalised filter resonance, 0..1.
};
inline constexpr float badgeInset = 0.02f, badgeRadius = 0.22f, badgeEdgeWidth = 0.035f;
inline constexpr float waveWidth = 0.05f, waveBaseline = 0.44f, wavePeak = 0.19f;
inline constexpr float waveLeft = 0.13f, waveRight = 0.87f;
inline constexpr int waveCycles = 3;
inline constexpr float slopeWidth = 0.038f, slopeDotRadius = 0.055f;
inline constexpr float knobRadius = 0.088f, knobRingWidth = 0.036f, knobNotchWidth = 0.042f;
inline const juce::Point<float> slopeStart { 0.13f, 0.54f }, slopeEnd { 0.87f, 0.72f };
inline const juce::Point<float> knobCentre { 0.22f, 0.79f }, knobNotch { 0.282f, 0.728f };

inline constexpr juce::uint32 badgeFill = 0xff10161f, badgeEdge = 0xff33425a;
inline constexpr juce::uint32 waveColour = 0xffb8ff38, slopeColour = 0xff42e8da, knobColour = 0xffb290ff;

// Top of the resonance parameter range, so a fully open filter maps to 1.
inline constexpr float maxResonanceParameter = 0.99f;
// Fraction of one saw cycle the tempo echo travels per beat.
inline constexpr float echoTravel = 0.34f;
// How far the badge edge brightens toward the acid green on the beat.
inline constexpr float beatGlow = 0.45f;
// Mark growth at the peak of the beat, and the filter opening up under resonance.
// The growth is capped so the swollen badge still fits inside the target rect.
inline constexpr float beatGrowth = 0.018f, resonanceStrokeGain = 0.6f, resonanceDotGain = 0.8f;

inline float resonanceAmount(float parameterValue)
{
    return juce::jlimit(0.f, 1.f, juce::jmax(0.f, parameterValue) / maxResonanceParameter);
}

// Advances the one-beat cycle by wall-clock seconds at the given tempo: at 120 BPM half a
// second is exactly one beat and the phase wraps back to zero.
inline AnimationState advance(AnimationState state, double beatsPerMinute, double seconds)
{
    const auto beats = juce::jmax(0.0, beatsPerMinute) * juce::jmax(0.0, seconds) / 60.0;
    return { static_cast<float>(std::fmod(static_cast<double>(state.phase) + beats, 1.0)),
             juce::jlimit(0.f, 1.f, state.resonance) };
}

// Palette used by the automated logo/raster comparison.
inline std::array<juce::uint32, 5> palette() { return { badgeFill, badgeEdge, waveColour, slopeColour, knobColour }; }

inline juce::Point<float> toAbsolute(juce::Point<float> normalised, juce::Rectangle<float> area)
{
    return { area.getX() + normalised.x * area.getWidth(), area.getY() + normalised.y * area.getHeight() };
}

inline std::vector<juce::Point<float>> wavePoints(juce::Rectangle<float> area)
{
    const float span = (waveRight - waveLeft) / static_cast<float>(waveCycles);
    std::vector<juce::Point<float>> points;
    points.push_back(toAbsolute({ waveLeft, waveBaseline }, area));
    for (int cycle = 0; cycle < waveCycles; ++cycle)
    {
        const float end = waveLeft + span * static_cast<float>(cycle + 1);
        points.push_back(toAbsolute({ end - 0.02f, wavePeak }, area));
        points.push_back(toAbsolute({ end - 0.02f, waveBaseline }, area));
    }
    return points;
}

inline juce::Path roundedBadge(juce::Rectangle<float> area)
{
    juce::Path path;
    path.addRoundedRectangle(area.reduced(area.getWidth() * badgeInset), area.getWidth() * badgeRadius);
    return path;
}

inline void paint(juce::Graphics& g, juce::Rectangle<float> target)
{
    if (target.getWidth() < 4 || target.getHeight() < 4)
        return;
    const auto scale = juce::jmin(target.getWidth(), target.getHeight());
    const auto area = target.withSizeKeepingCentre(scale, scale);
    const auto stroke = [scale](float fraction) { return juce::PathStrokeType(scale * fraction, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };

    g.setColour(juce::Colour(badgeFill)); g.fillPath(roundedBadge(area));
    g.setColour(juce::Colour(badgeEdge)); g.strokePath(roundedBadge(area), stroke(badgeEdgeWidth));

    const auto points = wavePoints(area);
    juce::Path wave;
    for (size_t index = 0; index < points.size(); ++index)
    {
        if (index == 0) wave.startNewSubPath(points[index]); else wave.lineTo(points[index]);
    }
    g.setColour(juce::Colour(waveColour)); g.strokePath(wave, stroke(waveWidth));

    juce::Path slope;
    slope.startNewSubPath(toAbsolute(slopeStart, area));
    slope.lineTo(toAbsolute(slopeEnd, area));
    g.setColour(juce::Colour(slopeColour));
    g.strokePath(slope, stroke(slopeWidth));
    const auto dot = toAbsolute(slopeEnd, area);
    g.fillEllipse(juce::Rectangle<float>(dot.x - scale * slopeDotRadius, dot.y - scale * slopeDotRadius,
                                         scale * slopeDotRadius * 2, scale * slopeDotRadius * 2));

    g.setColour(juce::Colour(knobColour));
    const auto knob = toAbsolute(knobCentre, area);
    g.drawEllipse(knob.x - scale * knobRadius, knob.y - scale * knobRadius,
                  scale * knobRadius * 2, scale * knobRadius * 2, scale * knobRingWidth);
    juce::Path notch;
    notch.startNewSubPath(toAbsolute(knobCentre, area));
    notch.lineTo(toAbsolute(knobNotch, area));
    g.strokePath(notch, stroke(knobNotchWidth));
}

// Draws the mark with a tempo-locked pulse and a resonance response:
//   * the badge swells slightly on every beat, and the edge brightens toward the acid green,
//   * an echo of the saw travels one third of a cycle per beat, so the mark loops seamlessly,
//   * resonance fattens the filter slope and grows its end dot, like the filter opening up.
// Everything is clipped to the badge, so the animation never spills past its header bounds.
inline void paint(juce::Graphics& g, juce::Rectangle<float> target, const AnimationState& state)
{
    if (target.getWidth() < 4 || target.getHeight() < 4)
        return;
    const auto phase = juce::jlimit(0.f, 1.f, state.phase);
    const auto resonance = juce::jlimit(0.f, 1.f, state.resonance);
    const auto scale = juce::jmin(target.getWidth(), target.getHeight());
    const auto pulse = juce::jmax(0.f, std::cos(phase * juce::MathConstants<float>::twoPi));
    const auto grown = scale * (1.f + beatGrowth * pulse);
    paint(g, target.withSizeKeepingCentre(grown, grown));

    const auto area = target.withSizeKeepingCentre(scale, scale);
    const auto stroke = [scale](float fraction) { return juce::PathStrokeType(scale * fraction, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };
    const juce::Graphics::ScopedSaveState unchanged(g);
    g.reduceClipRegion(roundedBadge(area.withSizeKeepingCentre(grown, grown)));

    // Each layer is skipped when it contributes nothing, so the animated mark never re-draws a
    // stroke just to leave the image unchanged. There is no phase where the whole mark is inert:
    // the pulse peaks on the beat while the echo is travelling, so the shipped raster is the
    // geometry rather than a frame of the animation.
    if (pulse > 0.f)
    {
        g.setColour(juce::Colour(badgeEdge).interpolatedWith(juce::Colour(waveColour), beatGlow * pulse));
        g.strokePath(roundedBadge(area.withSizeKeepingCentre(grown, grown)), stroke(badgeEdgeWidth * (1.f + 0.5f * pulse)));
    }

    // Tempo echo: the saw shifted right by a fraction of one cycle, fading in as it leaves.
    if (phase > 0.f)
    {
        const auto cycle = (waveRight - waveLeft) / static_cast<float>(waveCycles);
        const auto points = wavePoints(area);
        juce::Path echo;
        for (size_t index = 0; index < points.size(); ++index)
        {
            const auto point = points[index] + juce::Point<float>(area.getWidth() * cycle * echoTravel * phase, 0.f);
            if (index == 0) echo.startNewSubPath(point); else echo.lineTo(point);
        }
        g.setColour(juce::Colour(waveColour).withAlpha(0.75f * phase));
        g.strokePath(echo, stroke(waveWidth));
    }

    // Resonance: the filter slope steepens out and its dot opens up.
    if (resonance > 0.f)
    {
        juce::Path slope;
        slope.startNewSubPath(toAbsolute(slopeStart, area));
        slope.lineTo(toAbsolute(slopeEnd, area));
        g.setColour(juce::Colour(slopeColour));
        g.strokePath(slope, stroke(slopeWidth * (1.f + resonanceStrokeGain * resonance)));
        const auto dot = toAbsolute(slopeEnd, area);
        const auto dotRadius = scale * slopeDotRadius * (1.f + resonanceDotGain * resonance);
        g.setColour(juce::Colour(slopeColour).withAlpha(0.35f * resonance));
        g.fillEllipse(juce::Rectangle<float>(dot.x - dotRadius * 1.9f, dot.y - dotRadius * 1.9f, dotRadius * 3.8f, dotRadius * 3.8f));
        g.setColour(juce::Colour(slopeColour));
        g.fillEllipse(juce::Rectangle<float>(dot.x - dotRadius, dot.y - dotRadius, dotRadius * 2, dotRadius * 2));
    }
}
}