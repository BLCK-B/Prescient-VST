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

constexpr auto LOCAL_DEV_SERVER_ADDRESS = "http://127.0.0.1:8080";

//==============================================================================
MyAudioProcessorEditor::MyAudioProcessorEditor(MyAudioProcessor &p)
        : AudioProcessorEditor(&p), processorRef(p),
          webGainRelay{
                  webView, "GAIN"
          },
          webGainSliderAttachment{
                  *processorRef.treeState.getParameter("GAIN"), webGainRelay, nullptr
          },
          webView{juce::WebBrowserComponent::Options{}
                          .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                          .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                                          .withBackgroundColour(juce::Colours::white)
                                                          .withUserDataFolder(juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)))
                          .withNativeIntegrationEnabled()
                          .withResourceProvider([this](const auto &url) { return getResource(url); },juce::URL{LOCAL_DEV_SERVER_ADDRESS}.getOrigin())
                          .withOptionsFrom(webGainRelay)
          } {
    juce::ignoreUnused(processorRef);

    addAndMakeVisible(webView);

    webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable(true, true);
    setSize(800, 600);
}

MyAudioProcessorEditor::~MyAudioProcessorEditor() { }

//==============================================================================

auto MyAudioProcessorEditor::getResource(const juce::String& url) const -> std::optional<Resource> {
    static const auto resourceFileRoot = juce::File{"C:/Users/legionntb/Desktop/AudioPlugin/GUI/public"};
//    static const auto resourceFileRoot = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile)
//                    .getChildFile("public");

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
