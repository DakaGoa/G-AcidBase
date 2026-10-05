#pragma once
#include <JuceHeader.h>
#include "UpdateCheck.h"

// The About card: what this is, which build it is, and where to find it.
//
// Same furniture as the update result overlay - tinted backdrop, centred card,
// close on the card and on the backdrop - because both are the editor's own
// children and the point of this one is that every row is a clickable link,
// which a native message box cannot host.
//
// The three addresses live here as constants rather than being read from
// anywhere else: they are the product page, the source repository and the
// current release, and tests assert each row points at exactly one of them.
namespace gacid
{
inline constexpr const char* repositoryUrl = "https://github.com/Y4m4/G-AcidBase";
// /releases/latest, not /releases: the row is named "Current release", and that
// is the address that resolves to exactly the newest one rather than the list
// somebody then has to pick through.
inline constexpr const char* releasesUrl = "https://github.com/Y4m4/G-AcidBase/releases/latest";

struct AboutCard : juce::Component
{
    AboutCard()
    {
        addAndMakeVisible (titleLabel);
        addAndMakeVisible (versionLabel);
        addAndMakeVisible (summaryLabel);
        for (auto* link : { &pageLink, &sourceLink, &releaseLink })
        {
            addAndMakeVisible (*link);
            link->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff42e8da));
            link->setJustificationType (juce::Justification::centredLeft);
        }
        addAndMakeVisible (licenceLabel);
        addAndMakeVisible (okButton);
        addAndMakeVisible (closeButton);

        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb8ff38));
        versionLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb290ff));
        summaryLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe8eef7));
        licenceLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8798ac));

        // Every link is named by what it is, with the address behind it: the
        // URLs are far too long to print on a card this size.
        pageLink.setURL (juce::URL (siteUrl));
        pageLink.setButtonText ("Product page");
        pageLink.setTooltip (siteUrl);
        sourceLink.setURL (juce::URL (repositoryUrl));
        sourceLink.setButtonText ("Source repository");
        sourceLink.setTooltip (repositoryUrl);
        releaseLink.setURL (juce::URL (releasesUrl));
        releaseLink.setButtonText ("Current release");
        releaseLink.setTooltip (releasesUrl);

        versionLabel.setText ("Version " + versionString(), juce::dontSendNotification);
        summaryLabel.setText ("Goa acid bass synthesizer \u2014 303 reimagined.\n"
                              "Windows x64: VST3 instrument and standalone, 50 factory presets.",
                              juce::dontSendNotification);
        licenceLabel.setText ("JUCE modules are dual-licensed AGPLv3 or a commercial JUCE licence.\n"
                              "VST is a trademark of Steinberg Media Technologies GmbH.",
                              juce::dontSendNotification);

        okButton.onClick = [this] { setVisible (false); };
        closeButton.onClick = [this] { setVisible (false); };
        mouseDownCallback = [this] (const juce::MouseEvent&) { setVisible (false); };
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (mouseDownCallback && ! cardBounds().contains (e.getPosition()))
            mouseDownCallback (e);
    }

    // The centred card every row is laid out against.
    juce::Rectangle<int> cardBounds() const noexcept
    {
        return getLocalBounds().withSizeKeepingCentre (520, 380);
    }

    void resized() override
    {
        auto area = cardBounds().reduced (24);
        titleLabel.setBounds (area.removeFromTop (26));
        area.removeFromTop (2);
        versionLabel.setBounds (area.removeFromTop (20));
        area.removeFromTop (6);
        summaryLabel.setBounds (area.removeFromTop (44));
        area.removeFromTop (10);
        pageLink.setBounds (area.removeFromTop (24));
        sourceLink.setBounds (area.removeFromTop (24));
        releaseLink.setBounds (area.removeFromTop (24));
        area.removeFromTop (10);
        licenceLabel.setBounds (area.removeFromTop (40));
        area.removeFromTop (10);
        auto buttons = area.removeFromTop (28);
        okButton.setBounds (buttons.removeFromRight (120));
        buttons.removeFromRight (10);
        closeButton.setBounds (buttons.removeFromLeft (120));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.62f));
        auto card = cardBounds().toFloat();
        g.setColour (juce::Colour (0xff141d29));
        g.fillRoundedRectangle (card, 12.0f);
        g.setColour (juce::Colour (0xff283749));
        g.drawRoundedRectangle (card.reduced (0.5f), 12.0f, 1.0f);
    }

    juce::Label titleLabel, versionLabel, summaryLabel, licenceLabel;
    juce::HyperlinkButton pageLink, sourceLink, releaseLink;
    juce::TextButton okButton { "OK", "Dismiss the about card" };
    juce::TextButton closeButton { "\u00d7", "Back to the synth" };

private:
    std::function<void (const juce::MouseEvent&)> mouseDownCallback;
};
}
