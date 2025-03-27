#include "clay.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "raylib.h"  // For SetClipboardText()

// -----------------------
// Basic UI rendering functions
// -----------------------

const int FONT_ID_BODY_16 = 0;
Clay_Color COLOR_WHITE = { 255, 255, 255, 255 };

void RenderHeaderButton(Clay_String text) {
    CLAY({
        .layout = { .padding = { 16, 16, 8, 8 } },
        .backgroundColor = { 140, 140, 140, 255 },
        .cornerRadius = CLAY_CORNER_RADIUS(5)
    }) {
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
            .fontId = FONT_ID_BODY_16,
            .fontSize = 16,
            .textColor = { 255, 255, 255, 255 }
        }));
    }
}

void RenderDropdownMenuItem(Clay_String text) {
    CLAY({ .layout = { .padding = CLAY_PADDING_ALL(16) } }) {
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
            .fontId = FONT_ID_BODY_16,
            .fontSize = 16,
            .textColor = { 255, 255, 255, 255 }
        }));
    }
}

// -----------------------
// Data Structures
// -----------------------

typedef struct {
    Clay_String title;
    Clay_String contents;
} Document;

typedef struct {
    Document *documents;
    uint32_t length;
} DocumentArray;

Document documentsRaw[5];

DocumentArray documents = {
    .length = 5,
    .documents = documentsRaw
};

typedef struct {
    intptr_t offset;
    intptr_t memory;
} ClayVideoDemo_Arena;

typedef struct {
    int32_t selectedDocumentIndex;
    float yOffset;
    ClayVideoDemo_Arena frameArena;
    bool sidebarVisible;   // Field to track sidebar visibility
} ClayVideoDemo_Data;

typedef struct {
    int32_t requestedDocumentIndex;
    int32_t* selectedDocumentIndex;
} SidebarClickData;

void HandleSidebarInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData
) {
    SidebarClickData *clickData = (SidebarClickData*)userData;
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (clickData->requestedDocumentIndex >= 0 && clickData->requestedDocumentIndex < documents.length) {
            *clickData->selectedDocumentIndex = clickData->requestedDocumentIndex;
        }
    }
}

// Callback to toggle sidebar visibility when the Swap button is clicked.
void HandleSwapInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData
) {
    ClayVideoDemo_Data *data = (ClayVideoDemo_Data*)userData;
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        data->sidebarVisible = !data->sidebarVisible;
    }
}

// Use Clay_OnHover (since Clay_OnClick is not provided)
void RenderSwapButton(Clay_String text, ClayVideoDemo_Data *data) {
    CLAY({
        .id = CLAY_ID("SwapButton"),
        .layout = { .padding = { 16, 16, 8, 8 } },
        .backgroundColor = { 140, 140, 140, 255 },
        .cornerRadius = CLAY_CORNER_RADIUS(5)
    }) {
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
            .fontId = FONT_ID_BODY_16,
            .fontSize = 16,
            .textColor = { 255, 255, 255, 255 }
        }));
        Clay_OnHover(HandleSwapInteraction, (intptr_t)data);
    }
}

// -----------------------
// Text Selection and Highlighting
// -----------------------

// Structure to track text selection state.
typedef struct {
    bool isSelecting;
    int selectionStart;  // Character index where selection started
    int selectionEnd;    // Character index where selection ended
} TextSelection;

TextSelection gTextSelection = { false, 0, 0 };

// Dummy mapping: assume each character occupies 10 pixels in width.
int ComputeTextIndexFromPointer(float pointerX, const char *text) {
    int index = (int)(pointerX / 10.0f);
    int textLen = (int)strlen(text);
    if (index < 0) index = 0;
    if (index > textLen) index = textLen;
    return index;
}

// Callback for pointer events on the text element.
void HandleTextPointerEvent(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData) {
    TextSelection *selection = (TextSelection *)userData;
    // For this example we work on document 0's text.
    const char *text = documents.documents[0].contents.chars;

    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        selection->isSelecting = true;
        selection->selectionStart = ComputeTextIndexFromPointer(pointerData.position.x, text);
        selection->selectionEnd = selection->selectionStart;
    } else if (pointerData.state == CLAY_POINTER_DATA_PRESSED && selection->isSelecting) {
        selection->selectionEnd = ComputeTextIndexFromPointer(pointerData.position.x, text);
    } else if (pointerData.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
        selection->isSelecting = false;
    }
}

// Forward declaration for HandleCopyButton.
void HandleCopyButton(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData);

// Render a highlighted segment (assumes single-line text and dummy 10px per character).
void RenderHighlightedSegment(const char *segment) {
    int len = (int)strlen(segment);
    float width = len * 10.0f; // Dummy measurement: 10 pixels per character.
    float height = 30.0f;      // Fixed height for demonstration.

    // Render a rectangle as background highlight.
    CLAY({
        .layout = { .sizing = { CLAY_SIZING_FIXED(width), CLAY_SIZING_FIXED(height) } },
        .backgroundColor = { 255, 255, 0, 128 } // Semi-transparent yellow.
    }) {}
    // Render the text on top.
    {
        Clay_String temp = { .chars = segment, .length = len };
        CLAY_TEXT(temp, CLAY_TEXT_CONFIG({
            .fontId = FONT_ID_BODY_16,
            .fontSize = 24,
            .textColor = COLOR_WHITE
        }));
    }
}

// Render the document text with highlighting applied if a selection exists.
// For simplicity, we assume the text is a single line.
void RenderDocumentTextWithHighlight() {
    const char *fullText = documents.documents[0].contents.chars;
    int fullLen = (int)strlen(fullText);

    // If no selection or zero-length selection, render whole text.
    int selStart = gTextSelection.selectionStart;
    int selEnd = gTextSelection.selectionEnd;
    if (selStart > selEnd) {
        int tmp = selStart;
        selStart = selEnd;
        selEnd = tmp;
    }
    if (selStart == selEnd) {
        Clay_String temp = { .chars = fullText, .length = fullLen };
        CLAY_TEXT(temp, CLAY_TEXT_CONFIG({
            .fontId = FONT_ID_BODY_16,
            .fontSize = 24,
            .textColor = COLOR_WHITE
        }));
        return;
    }

    // Split text into three segments: pre-selection, selection, post-selection.
    int preLen = selStart;
    int selLen = selEnd - selStart;
    int postLen = fullLen - selEnd;

    const char *preText = fullText;
    const char *selText = fullText + selStart;
    const char *postText = fullText + selEnd;

    // Render these segments side-by-side using a horizontal layout container.
    CLAY({ .id = CLAY_ID("DocumentText_Horizontal"),
           .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT,
                       .childGap = 0 } }) {
        if (preLen > 0) {
            Clay_String preSegment = { .chars = preText, .length = preLen };
            CLAY_TEXT(preSegment, CLAY_TEXT_CONFIG({
                .fontId = FONT_ID_BODY_16,
                .fontSize = 24,
                .textColor = COLOR_WHITE
            }));
        }
        if (selLen > 0) {
            RenderHighlightedSegment(selText);
        }
        if (postLen > 0) {
            Clay_String postSegment = { .chars = postText, .length = postLen };
            CLAY_TEXT(postSegment, CLAY_TEXT_CONFIG({
                .fontId = FONT_ID_BODY_16,
                .fontSize = 24,
                .textColor = COLOR_WHITE
            }));
        }
    }
}

// Callback for the Copy button.
void HandleCopyButton(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData) {
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        TextSelection *selection = (TextSelection *)userData;
        const char *text = documents.documents[0].contents.chars;
        int start = selection->selectionStart;
        int end = selection->selectionEnd;
        if (start > end) {
            int tmp = start;
            start = end;
            end = tmp;
        }
        int len = end - start;
        if (len > 0) {
            char *copyBuffer = (char*)malloc(len + 1);
            strncpy(copyBuffer, text + start, len);
            copyBuffer[len] = '\0';
            SetClipboardText(copyBuffer);
            free(copyBuffer);
        }
    }
}

// Render a Copy button that copies the selected text.
void RenderCopyButton() {
    CLAY({
        .id = CLAY_ID("CopyButton"),
        .layout = { .padding = CLAY_PADDING_ALL(16) },
        .backgroundColor = {120, 120, 120, 255},
        .cornerRadius = CLAY_CORNER_RADIUS(5)
    }) {
        CLAY_TEXT(CLAY_STRING("Copy"), CLAY_TEXT_CONFIG({
            .fontId = FONT_ID_BODY_16,
            .fontSize = 16,
            .textColor = {255, 255, 255, 255}
        }));
        Clay_OnHover(HandleCopyButton, (intptr_t)&gTextSelection);
    }
}

// -----------------------
// Main Layout functions
// -----------------------

ClayVideoDemo_Data ClayVideoDemo_Initialize() {
    documents.documents[0] = (Document){
        .title = CLAY_STRING("Squirrels"),
        .contents = CLAY_STRING("The Secret Life of Squirrels: Nature's Clever Acrobats\n"
            "Squirrels are often overlooked creatures, dismissed as mere park inhabitants or backyard nuisances. Yet, beneath their fluffy tails and twitching noses lies an intricate world of cunning, agility, and survival tactics that are nothing short of fascinating. As one of the most common mammals in North America, squirrels have adapted to a wide range of environments from bustling urban centers to tranquil forests and have developed a variety of unique behaviors that continue to intrigue scientists and nature enthusiasts alike.\n"
            "\n"
            "Master Tree Climbers\n"
            "At the heart of a squirrel's skill set is its impressive ability to navigate trees with ease. Whether they're darting from branch to branch or leaping across wide gaps, squirrels possess an innate talent for acrobatics. Their powerful hind legs, which are longer than their front legs, give them remarkable jumping power. With a tail that acts as a counterbalance, squirrels can leap distances of up to ten times the length of their body, making them some of the best aerial acrobats in the animal kingdom.\n"
            "But it's not just their agility that makes them exceptional climbers. Squirrels' sharp, curved claws allow them to grip tree bark with precision, while the soft pads on their feet provide traction on slippery surfaces. Their ability to run at high speeds and scale vertical trunks with ease is a testament to the evolutionary adaptations that have made them so successful in their arboreal habitats.\n"
            "\n"
            "Food Hoarders Extraordinaire\n"
            "Squirrels are often seen frantically gathering nuts, seeds, and even fungi in preparation for winter. While this behavior may seem like instinctual hoarding, it is actually a survival strategy that has been honed over millions of years. Known as \"scatter hoarding,\" squirrels store their food in a variety of hidden locations, often burying it deep in the soil or stashing it in hollowed-out tree trunks.\n"
            "Interestingly, squirrels have an incredible memory for the locations of their caches. Research has shown that they can remember thousands of hiding spots, often returning to them months later when food is scarce. However, they don't always recover every stash; some forgotten caches eventually sprout into new trees, contributing to forest regeneration. This unintentional role as forest gardeners highlights the ecological importance of squirrels in their ecosystems.\n"
            "\n"
            "The Great Squirrel Debate: Urban vs. Wild\n"
            "While squirrels are most commonly associated with rural or wooded areas, their adaptability has allowed them to thrive in urban environments as well. In cities, squirrels have become adept at finding food sources in places like parks, streets, and even garbage cans. However, their urban counterparts face unique challenges, including traffic, predators, and the lack of natural shelters. Despite these obstacles, squirrels in urban areas are often observed using human infrastructure such as buildings, bridges, and power lines as highways for their acrobatic escapades.\n"
            "\n"
            "A Symbol of Resilience\n"
            "In many cultures, squirrels are symbols of resourcefulness, adaptability, and preparation. Their ability to thrive in a variety of environments while navigating challenges with agility and grace serves as a reminder of the resilience inherent in nature. Whether you encounter them in a quiet forest, a city park, or your own backyard, squirrels are creatures that never fail to amaze with their endless energy and ingenuity.\n"
            "In the end, squirrels may be small, but they are mighty in their ability to survive and thrive in a world that is constantly changing. So next time you spot one hopping across a branch or darting across your lawn, take a moment to appreciate the remarkable acrobat at work—a true marvel of the natural world.\n")
    };
    documents.documents[1] = (Document){
        .title = CLAY_STRING("Lorem Ipsum"),
        .contents = CLAY_STRING("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.")
    };
    documents.documents[2] = (Document){
        .title = CLAY_STRING("Vacuum Instructions"),
        .contents = CLAY_STRING("Chapter 3: Getting Started - Unpacking and Setup\n"
            "\n"
            "Congratulations on your new SuperClean Pro 5000 vacuum cleaner! In this section, we will guide you through the simple steps to get your vacuum up and running. Before you begin, please ensure that you have all the components listed in the \"Package Contents\" section on page 2.\n"
            "\n"
            "1. Unboxing Your Vacuum\n"
            "Carefully remove the vacuum cleaner from the box. Avoid using sharp objects that could damage the product. Once removed, place the unit on a flat, stable surface to proceed with the setup. Inside the box, you should find:\n"
            "\n"
            "    The main vacuum unit\n"
            "    A telescoping extension wand\n"
            "    A set of specialized cleaning tools (crevice tool, upholstery brush, etc.)\n"
            "    A reusable dust bag (if applicable)\n"
            "    A power cord with a 3-prong plug\n"
            "    A set of quick-start instructions\n"
            "\n"
            "2. Assembling Your Vacuum\n"
            "Begin by attaching the extension wand to the main body of the vacuum cleaner. Line up the connectors and twist the wand into place until you hear a click. Next, select the desired cleaning tool and firmly attach it to the wand's end, ensuring it is securely locked in.\n"
            "\n"
            "For models that require a dust bag, slide the bag into the compartment at the back of the vacuum, making sure it is properly aligned with the internal mechanism. If your vacuum uses a bagless system, ensure the dust container is correctly seated and locked in place before use.\n"
            "\n"
            "3. Powering On\n"
            "To start the vacuum, plug the power cord into a grounded electrical outlet. Once plugged in, locate the power switch, usually positioned on the side of the handle or body of the unit, depending on your model. Press the switch to the \"On\" position, and you should hear the motor begin to hum. If the vacuum does not power on, check that the power cord is securely plugged in, and ensure there are no blockages in the power switch.\n"
            "\n"
            "Note: Before first use, ensure that the vacuum filter (if your model has one) is properly installed. If unsure, refer to \"Section 5: Maintenance\" for filter installation instructions.")
    };
    documents.documents[3] = (Document){
        .title = CLAY_STRING("Article 4"),
        .contents = CLAY_STRING("Article 4 Content")
    };
    documents.documents[4] = (Document){
        .title = CLAY_STRING("Article 5"),
        .contents = CLAY_STRING("Article 5 Content")
    };

    ClayVideoDemo_Data data = {
        .selectedDocumentIndex = 0,
        .yOffset = 0,
        .frameArena = { .memory = (intptr_t)malloc(1024), .offset = 0 },
        .sidebarVisible = true   // Sidebar starts visible
    };
    return data;
}

Clay_RenderCommandArray ClayVideoDemo_CreateLayout(ClayVideoDemo_Data *data) {
    data->frameArena.offset = 0;
    Clay_BeginLayout();

    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    Clay_Color contentBackgroundColor = { 90, 90, 90, 255 };

    CLAY({ .id = CLAY_ID("OuterContainer"),
        .backgroundColor = {43, 41, 51, 255 },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = layoutExpand,
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16
        }
    }) {
        CLAY({ .id = CLAY_ID("HeaderBar"),
            .layout = {
                .sizing = {
                    .height = CLAY_SIZING_FIXED(60),
                    .width = CLAY_SIZING_GROW(0)
                },
                .padding = { 16, 16, 0, 0 },
                .childGap = 16,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = contentBackgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            CLAY({ .id = CLAY_ID("FileButton"),
                .layout = { .padding = { 16, 16, 8, 8 } },
                .backgroundColor = {140, 140, 140, 255},
                .cornerRadius = CLAY_CORNER_RADIUS(5)
            }) {
                CLAY_TEXT(CLAY_STRING("File"), CLAY_TEXT_CONFIG({
                    .fontId = FONT_ID_BODY_16,
                    .fontSize = 16,
                    .textColor = { 255, 255, 255, 255 }
                }));

                bool fileMenuVisible =
                    Clay_PointerOver(Clay_GetElementId(CLAY_STRING("FileButton"))) ||
                    Clay_PointerOver(Clay_GetElementId(CLAY_STRING("FileMenu")));

                if (fileMenuVisible) {
                    CLAY({ .id = CLAY_ID("FileMenu"),
                        .floating = {
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                            .attachPoints = { .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM },
                        },
                        .layout = { .padding = {0, 0, 8, 8 } }
                    }) {
                        CLAY({
                            .layout = {
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                .sizing = { .width = CLAY_SIZING_FIXED(200) },
                            },
                            .backgroundColor = {40, 40, 40, 255},
                            .cornerRadius = CLAY_CORNER_RADIUS(8)
                        }) {
                            RenderDropdownMenuItem(CLAY_STRING("New"));
                            RenderDropdownMenuItem(CLAY_STRING("Open"));
                            RenderDropdownMenuItem(CLAY_STRING("Close"));
                        }
                    }
                }
            }
            RenderSwapButton(CLAY_STRING("Swap"), data);
            CLAY({ .layout = { .sizing = { CLAY_SIZING_GROW(0) } } }) {}
            RenderHeaderButton(CLAY_STRING("Upload"));
            RenderHeaderButton(CLAY_STRING("Media"));
            RenderHeaderButton(CLAY_STRING("Support"));
        }

        CLAY({ .id = CLAY_ID("LowerContent"),
            .layout = { .sizing = layoutExpand, .childGap = 16 }
        }) {
            if (data->sidebarVisible) {
                CLAY({ .id = CLAY_ID("Sidebar"),
                    .backgroundColor = contentBackgroundColor,
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = CLAY_PADDING_ALL(16),
                        .childGap = 8,
                        .sizing = { .width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_GROW(0) }
                    }
                }) {
                    for (int i = 0; i < documents.length; i++) {
                        Document document = documents.documents[i];
                        Clay_LayoutConfig sidebarButtonLayout = {
                            .sizing = { .width = CLAY_SIZING_GROW(0) },
                            .padding = CLAY_PADDING_ALL(16)
                        };

                        if (i == data->selectedDocumentIndex) {
                            CLAY({
                                .layout = sidebarButtonLayout,
                                .backgroundColor = {120, 120, 120, 255},
                                .cornerRadius = CLAY_CORNER_RADIUS(8)
                            }) {
                                CLAY_TEXT(document.title, CLAY_TEXT_CONFIG({
                                    .fontId = FONT_ID_BODY_16,
                                    .fontSize = 20,
                                    .textColor = { 255, 255, 255, 255 }
                                }));
                            }
                        } else {
                            SidebarClickData *clickData = (SidebarClickData *)(data->frameArena.memory + data->frameArena.offset);
                            *clickData = (SidebarClickData){ .requestedDocumentIndex = i, .selectedDocumentIndex = &data->selectedDocumentIndex };
                            data->frameArena.offset += sizeof(SidebarClickData);
                            CLAY({ .layout = sidebarButtonLayout,
                                .backgroundColor = (Clay_Color){ 120, 120, 120, Clay_Hovered() ? 120 : 0 },
                                .cornerRadius = CLAY_CORNER_RADIUS(8)
                            }) {
                                Clay_OnHover(HandleSidebarInteraction, (intptr_t)clickData);
                                CLAY_TEXT(document.title, CLAY_TEXT_CONFIG({
                                    .fontId = FONT_ID_BODY_16,
                                    .fontSize = 20,
                                    .textColor = { 255, 255, 255, 255 }
                                }));
                            }
                        }
                    }
                }
            }

            CLAY({ .id = CLAY_ID("MainContent"),
                .backgroundColor = contentBackgroundColor,
                .scroll = { .vertical = true },
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap = 16,
                    .padding = CLAY_PADDING_ALL(16),
                    .sizing = layoutExpand
                }
            }) {
                Document selectedDocument = documents.documents[data->selectedDocumentIndex];
                CLAY_TEXT(selectedDocument.title, CLAY_TEXT_CONFIG({
                    .fontId = FONT_ID_BODY_16,
                    .fontSize = 24,
                    .textColor = COLOR_WHITE
                }));
                // For document 0, render text with selection highlighting and a Copy button.
                if (data->selectedDocumentIndex == 0) {
                    RenderDocumentTextWithHighlight();
                    RenderCopyButton();
                } else {
                    CLAY_TEXT(selectedDocument.contents, CLAY_TEXT_CONFIG({
                        .fontId = FONT_ID_BODY_16,
                        .fontSize = 24,
                        .textColor = COLOR_WHITE
                    }));
                }
            }
        }
    }

    Clay_RenderCommandArray renderCommands = Clay_EndLayout();
    for (int32_t i = 0; i < renderCommands.length; i++) {
        Clay_RenderCommandArray_Get(&renderCommands, i)->boundingBox.y += data->yOffset;
    }
    return renderCommands;
}
