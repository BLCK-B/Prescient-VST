#include "PluginEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>
#include "PluginProcessor.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_extra/juce_gui_extra.h"

namespace {
    std::vector<std::byte> streamToVector(juce::InputStream& stream) {
        // Workaround to make ssize_t work cross-platform.
        using namespace juce;
        const auto sizeInBytes = static_cast<size_t>(stream.getTotalLength());
        std::vector<std::byte> result(sizeInBytes);
        stream.setPosition(0);
        [[maybe_unused]] const auto bytesRead =
                stream.read(result.data(), result.size());
        jassert(bytesRead == static_cast<ssize_t>(sizeInBytes));
        return result;
    }
}

static const char* getMimeForExtension (const juce::String& extension) {
    using namespace juce;
    static const std::unordered_map<String, const char*> mimeMap = {
            { { "htm"   },  "text/html"                },
            { { "html"  },  "text/html"                },
            { { "txt"   },  "text/plain"               },
            { { "jpg"   },  "image/jpeg"               },
            { { "jpeg"  },  "image/jpeg"               },
            { { "svg"   },  "image/svg+xml"            },
            { { "ico"   },  "image/vnd.microsoft.icon" },
            { { "json"  },  "application/json"         },
            { { "png"   },  "image/png"                },
            { { "css"   },  "text/css"                 },
            { { "map"   },  "application/json"         },
            { { "js"    },  "text/javascript"          },
            { { "woff2" },  "font/woff2"               }};

    if (const auto it = mimeMap.find(extension.toLowerCase());
            it != mimeMap.end())
        return it->second;

    jassertfalse;
    return "";
}

//==============================================================================
MyAudioProcessorEditor::MyAudioProcessorEditor(MyAudioProcessor &p)
        : AudioProcessorEditor(&p), processorRef(p),
          modelOrderRelay{
                  webView, "modelOrder"
          },
          passthroughRelay{
                  webView, "passthrough"
          },
          shiftVoice1Relay{
                  webView, "shiftVoice1"
          },
          shiftVoice2Relay{
                  webView, "shiftVoice2"
          },
          shiftVoice3Relay{
                  webView, "shiftVoice3"
          },
          monostereoRelay{
                  webView, "monostereo"
          },
          enableLPCRelay{
                  webView, "enableLPC"
          },
          webView{juce::WebBrowserComponent::Options{}
                          .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                          .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                                          .withBackgroundColour(juce::Colours::white)
                                                          .withUserDataFolder(juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)))
                          .withNativeIntegrationEnabled()
                          .withResourceProvider([](const auto &url) { return getResource(url); })
                          .withOptionsFrom(modelOrderRelay)
                          .withOptionsFrom(passthroughRelay)
                          .withOptionsFrom(shiftVoice1Relay)
                          .withOptionsFrom(shiftVoice2Relay)
                          .withOptionsFrom(shiftVoice3Relay)
                          .withOptionsFrom(monostereoRelay)
                          .withOptionsFrom(enableLPCRelay)
          },
          modelOrderSliderAttachment{
                  *processorRef.treeState.getParameter("modelOrder"), modelOrderRelay, nullptr
          },
          passthroughSliderAttachment{
                  *processorRef.treeState.getParameter("passthrough"), passthroughRelay, nullptr
          },
          shiftVoice1SliderAttachment{
                  *processorRef.treeState.getParameter("shiftVoice1"), shiftVoice1Relay, nullptr
          },
          shiftVoice2SliderAttachment{
                  *processorRef.treeState.getParameter("shiftVoice2"), shiftVoice2Relay, nullptr
          },
          shiftVoice3SliderAttachment{
                  *processorRef.treeState.getParameter("shiftVoice3"), shiftVoice3Relay, nullptr
          },
          monostereoSliderAttachment{
                  *processorRef.treeState.getParameter("monostereo"), monostereoRelay, nullptr
          },
          enableLPCSliderAttachment{
                  *processorRef.treeState.getParameter("enableLPC"),enableLPCRelay, nullptr
          }
{
    juce::ignoreUnused(processorRef);

    addAndMakeVisible(webView);

    webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable(false, false);
    setSize(610, 400);
}

MyAudioProcessorEditor::~MyAudioProcessorEditor() { }

//==============================================================================

auto MyAudioProcessorEditor::getResource(const juce::String& url) -> std::optional<Resource> {
    // vst3 searches for 'public' in 'Contents' that must be added each build
    static const auto resourceFileRoot =
            juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::currentApplicationFile)
                    .getParentDirectory()
                    .getParentDirectory()
                    .getChildFile("public");

    const auto resourceToRetrieve = url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);
    const auto resource = resourceFileRoot.getChildFile(resourceToRetrieve).createInputStream();
    if (resource) {
        const auto extension = resourceToRetrieve.fromLastOccurrenceOf(".", false, false);
        return Resource{streamToVector(*resource), getMimeForExtension(extension)};
    }
    return std::nullopt;
}


void MyAudioProcessorEditor::resized() {
    webView.setBounds(getLocalBounds());
}
