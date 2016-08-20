// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/imgui/imgui.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_MATH_OPERATORS
#include "third_party/imgui/imgui_internal.h"


struct GLFWwindow;

IMGUI_API bool        ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks);
IMGUI_API void        ImGui_ImplGlfw_Shutdown();
IMGUI_API void        ImGui_ImplGlfw_NewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplGlfw_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplGlfw_CreateDeviceObjects();

// GLFW callbacks (installed by default if you enable 'install_callbacks' during initialization)
// Provided here if you want to chain callbacks.
// You can also handle inputs yourself and use those as a reference.
IMGUI_API void        ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_API void        ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_API void        ImGui_ImplGlFw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_API void        ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);

// Forward declarations
typedef int ImGuiWindowFlags;

typedef enum ImGuiDockSlot {
    ImGuiDockSlot_Left,
    ImGuiDockSlot_Right,
    ImGuiDockSlot_Top,
    ImGuiDockSlot_Bottom,
    ImGuiDockSlot_Tab,

    ImGuiDockSlot_Float,
    ImGuiDockSlot_None
} ImGuiDockSlot;

namespace ImGui{

IMGUI_API void BeginDockspace();
IMGUI_API void EndDockspace();
IMGUI_API void ShutdownDock();
IMGUI_API void SetNextDock(ImGuiDockSlot slot);
IMGUI_API bool BeginDock(const char* label, bool* opened = NULL, ImGuiWindowFlags extra_flags = 0);
IMGUI_API void EndDock();
IMGUI_API void SetDockActive();
IMGUI_API void DockDebugWindow();

}  // namespace ImGui

// ImGui GLFW binding with OpenGL
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// If your context is GL3/GL3 then prefer using the code in opengl3_example.
// You *might* use this code with a GL3/GL4 context but make sure you disable the programmable pipeline by calling "glUseProgram(0)" before ImGui::Render().
// We cannot do that from GL2 code because the function doesn't exist.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// GLFW
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

// Data
static GLFWwindow*  g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGlfw_RenderDrawLists(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Render command lists
    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
        const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
    #undef OFFSETOF

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

static const char* ImGui_ImplGlfw_GetClipboardText()
{
    return glfwGetClipboardString(g_Window);
}

static void ImGui_ImplGlfw_SetClipboardText(const char* text)
{
    glfwSetClipboardString(g_Window, text);
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_MousePressed[button] = true;
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void ImGui_ImplGlFw_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

bool ImGui_ImplGlfw_CreateDeviceObjects()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

void    ImGui_ImplGlfw_InvalidateDeviceObjects()
{
    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

bool    ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks)
{
    g_Window = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImGui_ImplGlfw_RenderDrawLists;      // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

    if (install_callbacks)
    {
        glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
        glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
        glfwSetKeyCallback(window, ImGui_ImplGlFw_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    }

    return true;
}

void ImGui_ImplGlfw_Shutdown()
{
    ImGui_ImplGlfw_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplGlfw_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplGlfw_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        double mouse_x, mouse_y;
        glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
        io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}

/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
// based on https://github.com/nem0/LumixEngine/blob/master/external/imgui/imgui_dock.inl
// modified from https://bitbucket.org/duangle/liminal/src/tip/src/liminal/imgui_dock.cpp

using namespace ImGui;

struct DockContext {
  enum EndAction_ {
    EndAction_None,
    EndAction_Panel,
    EndAction_End,
    EndAction_EndChild
  };

  enum Status_ { Status_Docked, Status_Float, Status_Dragged };

  struct Dock {
    Dock()
        : label(nullptr),
          id(0),
          next_tab(nullptr),
          prev_tab(nullptr),
          parent(nullptr),
          active(true),
          pos(0, 0),
          size(-1, -1),
          status(Status_Float),
          opened(false)

    {
      location[0] = 0;
      children[0] = children[1] = nullptr;
    }

    ~Dock() { MemFree(label); }

    ImVec2 getMinSize() const {
      if (!children[0])
        return ImVec2(16, 16 + GetTextLineHeightWithSpacing());

      ImVec2 s0 = children[0]->getMinSize();
      ImVec2 s1 = children[1]->getMinSize();
      return isHorizontal() ? ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y))
                            : ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
    }

    bool isHorizontal() const {
      return children[0]->pos.x < children[1]->pos.x;
    }

    void setParent(Dock* dock) {
      parent = dock;
      for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab)
        tmp->parent = dock;
      for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab)
        tmp->parent = dock;
    }

    Dock& getRoot() {
      Dock* dock = this;
      while (dock->parent)
        dock = dock->parent;
      return *dock;
    }

    Dock& getSibling() {
      IM_ASSERT(parent);
      if (parent->children[0] == &getFirstTab())
        return *parent->children[1];
      return *parent->children[0];
    }

    Dock& getFirstTab() {
      Dock* tmp = this;
      while (tmp->prev_tab)
        tmp = tmp->prev_tab;
      return *tmp;
    }

    void setActive() {
      active = true;
      for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab)
        tmp->active = false;
      for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab)
        tmp->active = false;
    }

    bool isContainer() const { return children[0] != nullptr; }

    void setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size) {
      ImVec2 s = children[0]->size;
      if (isHorizontal()) {
        s.y = _size.y;
        s.x = (float)int(_size.x * children[0]->size.x /
                         (children[0]->size.x + children[1]->size.x));
        if (s.x < children[0]->getMinSize().x) {
          s.x = children[0]->getMinSize().x;
        } else if (_size.x - s.x < children[1]->getMinSize().x) {
          s.x = _size.x - children[1]->getMinSize().x;
        }
        children[0]->setPosSize(_pos, s);

        s.x = _size.x - children[0]->size.x;
        ImVec2 p = _pos;
        p.x += children[0]->size.x;
        children[1]->setPosSize(p, s);
      } else {
        s.x = _size.x;
        s.y = (float)int(_size.y * children[0]->size.y /
                         (children[0]->size.y + children[1]->size.y));
        if (s.y < children[0]->getMinSize().y) {
          s.y = children[0]->getMinSize().y;
        } else if (_size.y - s.y < children[1]->getMinSize().y) {
          s.y = _size.y - children[1]->getMinSize().y;
        }
        children[0]->setPosSize(_pos, s);

        s.y = _size.y - children[0]->size.y;
        ImVec2 p = _pos;
        p.y += children[0]->size.y;
        children[1]->setPosSize(p, s);
      }
    }

    void setPosSize(const ImVec2& _pos, const ImVec2& _size) {
      size = _size;
      pos = _pos;
      for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) {
        tmp->size = _size;
        tmp->pos = _pos;
      }
      for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) {
        tmp->size = _size;
        tmp->pos = _pos;
      }

      if (!isContainer())
        return;
      setChildrenPosSize(_pos, _size);
    }

    char* label;
    ImU32 id;
    Dock* next_tab;
    Dock* prev_tab;
    Dock* children[2];
    Dock* parent;
    bool active;
    ImVec2 pos;
    ImVec2 size;
    Status_ status;
    int last_frame;
    int invalid_frames;
    char location[16];
    bool opened;
    bool first;
  };

  ImVector<Dock*> m_docks;
  ImVec2 m_drag_offset;
  Dock* m_current;
  Dock* m_next_parent;
  int m_last_frame;
  EndAction_ m_end_action;
  ImVec2 m_workspace_pos;
  ImVec2 m_workspace_size;
  ImGuiDockSlot m_next_dock_slot;

  DockContext()
      : m_current(nullptr),
        m_next_parent(nullptr),
        m_last_frame(0),
        m_next_dock_slot(ImGuiDockSlot_Tab) {}

  ~DockContext() {}

  Dock& getDock(const char* label, bool opened) {
    ImU32 id = ImHash(label, 0);
    for (int i = 0; i < m_docks.size(); ++i) {
      if (m_docks[i]->id == id)
        return *m_docks[i];
    }

    Dock* new_dock = (Dock*)MemAlloc(sizeof(Dock));
    IM_PLACEMENT_NEW(new_dock) Dock();
    m_docks.push_back(new_dock);
    new_dock->label = ImStrdup(label);
    IM_ASSERT(new_dock->label);
    new_dock->id = id;
    new_dock->setActive();
    new_dock->status = (m_docks.size() == 1) ? Status_Docked : Status_Float;
    new_dock->pos = ImVec2(0, 0);
    new_dock->size = GetIO().DisplaySize;
    new_dock->opened = opened;
    new_dock->first = true;
    new_dock->last_frame = 0;
    new_dock->invalid_frames = 0;
    new_dock->location[0] = 0;
    return *new_dock;
  }

  void putInBackground() {
    ImGuiWindow* win = GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    if (g.Windows[0] == win)
      return;

    for (int i = 0; i < g.Windows.Size; i++) {
      if (g.Windows[i] == win) {
        for (int j = i - 1; j >= 0; --j) {
          g.Windows[j + 1] = g.Windows[j];
        }
        g.Windows[0] = win;
        break;
      }
    }
  }

  void splits() {
    if (GetFrameCount() == m_last_frame)
      return;
    m_last_frame = GetFrameCount();

    putInBackground();

    for (int i = 0; i < m_docks.size(); ++i) {
      Dock& dock = *m_docks[i];
      if (!dock.parent && (dock.status == Status_Docked)) {
        dock.setPosSize(m_workspace_pos, m_workspace_size);
      }
    }

    ImU32 color = GetColorU32(ImGuiCol_Button);
    ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);
    ImDrawList* draw_list = GetWindowDrawList();
    ImGuiIO& io = GetIO();
    for (int i = 0; i < m_docks.size(); ++i) {
      Dock& dock = *m_docks[i];
      if (!dock.isContainer())
        continue;

      PushID(i);
      if (!IsMouseDown(0))
        dock.status = Status_Docked;

      ImVec2 pos0 = dock.children[0]->pos;
      ImVec2 pos1 = dock.children[1]->pos;
      ImVec2 size0 = dock.children[0]->size;
      ImVec2 size1 = dock.children[1]->size;

      ImGuiMouseCursor cursor;

      ImVec2 dsize(0, 0);
      ImVec2 min_size0 = dock.children[0]->getMinSize();
      ImVec2 min_size1 = dock.children[1]->getMinSize();
      if (dock.isHorizontal()) {
        cursor = ImGuiMouseCursor_ResizeEW;
        SetCursorScreenPos(ImVec2(dock.pos.x + size0.x, dock.pos.y));
        InvisibleButton("split", ImVec2(3, dock.size.y));
        if (dock.status == Status_Dragged)
          dsize.x = io.MouseDelta.x;
        dsize.x = -ImMin(-dsize.x, dock.children[0]->size.x - min_size0.x);
        dsize.x = ImMin(dsize.x, dock.children[1]->size.x - min_size1.x);
        size0 += dsize;
        size1 -= dsize;
        pos0 = dock.pos;
        pos1.x = pos0.x + size0.x;
        pos1.y = dock.pos.y;
        size0.y = size1.y = dock.size.y;
        size1.x = ImMax(min_size1.x, dock.size.x - size0.x);
        size0.x = ImMax(min_size0.x, dock.size.x - size1.x);
      } else {
        cursor = ImGuiMouseCursor_ResizeNS;
        SetCursorScreenPos(ImVec2(dock.pos.x, dock.pos.y + size0.y));
        InvisibleButton("split", ImVec2(dock.size.x, 3));
        if (dock.status == Status_Dragged)
          dsize.y = io.MouseDelta.y;
        dsize.y = -ImMin(-dsize.y, dock.children[0]->size.y - min_size0.y);
        dsize.y = ImMin(dsize.y, dock.children[1]->size.y - min_size1.y);
        size0 += dsize;
        size1 -= dsize;
        pos0 = dock.pos;
        pos1.x = dock.pos.x;
        pos1.y = pos0.y + size0.y;
        size0.x = size1.x = dock.size.x;
        size1.y = ImMax(min_size1.y, dock.size.y - size0.y);
        size0.y = ImMax(min_size0.y, dock.size.y - size1.y);
      }
      dock.children[0]->setPosSize(pos0, size0);
      dock.children[1]->setPosSize(pos1, size1);

      if (IsItemHovered()) {
        SetMouseCursor(cursor);
      }

      if (IsItemHovered() && IsMouseClicked(0)) {
        dock.status = Status_Dragged;
      }

      draw_list->AddRectFilled(GetItemRectMin(),
                               GetItemRectMax(),
                               IsItemHovered() ? color_hovered : color);
      PopID();
    }
  }

  void checkNonexistent() {
    int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
    for (int i = 0; i < m_docks.size(); ++i) {
      Dock* dock = m_docks[i];
      if (dock->isContainer())
        continue;
      if (dock->status == Status_Float)
        continue;
      if (dock->last_frame < frame_limit) {
        ++dock->invalid_frames;
        if (dock->invalid_frames > 2) {
          doUndock(*dock);
          dock->status = Status_Float;
        }
        return;
      }
      dock->invalid_frames = 0;
    }
  }

  Dock* getDockAt(const ImVec2& pos) const {
    for (int i = 0; i < m_docks.size(); ++i) {
      Dock& dock = *m_docks[i];
      if (dock.isContainer())
        continue;
      if (dock.status != Status_Docked)
        continue;
      if (IsMouseHoveringRect(dock.pos, dock.pos + dock.size, false)) {
        return &dock;
      }
    }

    return nullptr;
  }

  static ImRect getDockedRect(const ImRect& rect, ImGuiDockSlot dock_slot) {
    ImVec2 half_size = rect.GetSize() * 0.5f;
    switch (dock_slot) {
      default:
        return rect;
      case ImGuiDockSlot_Top:
        return ImRect(rect.Min, ImVec2(rect.Max.x, rect.Min.y + half_size.y));
      case ImGuiDockSlot_Right:
        return ImRect(rect.Min + ImVec2(half_size.x, 0), rect.Max);
      case ImGuiDockSlot_Bottom:
        return ImRect(rect.Min + ImVec2(0, half_size.y), rect.Max);
      case ImGuiDockSlot_Left:
        return ImRect(rect.Min, ImVec2(rect.Min.x + half_size.x, rect.Max.y));
    }
  }

  static ImRect getSlotRect(ImRect parent_rect, ImGuiDockSlot dock_slot) {
    ImVec2 size = parent_rect.Max - parent_rect.Min;
    ImVec2 center = parent_rect.Min + size * 0.5f;
    switch (dock_slot) {
      default:
        return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
      case ImGuiDockSlot_Top:
        return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
      case ImGuiDockSlot_Right:
        return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
      case ImGuiDockSlot_Bottom:
        return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
      case ImGuiDockSlot_Left:
        return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
    }
  }

  static ImRect getSlotRectOnBorder(ImRect parent_rect,
                                    ImGuiDockSlot dock_slot) {
    ImVec2 size = parent_rect.Max - parent_rect.Min;
    ImVec2 center = parent_rect.Min + size * 0.5f;
    switch (dock_slot) {
      case ImGuiDockSlot_Top:
        return ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10),
                      ImVec2(center.x + 20, parent_rect.Min.y + 30));
      case ImGuiDockSlot_Left:
        return ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20),
                      ImVec2(parent_rect.Min.x + 30, center.y + 20));
      case ImGuiDockSlot_Bottom:
        return ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30),
                      ImVec2(center.x + 20, parent_rect.Max.y - 10));
      case ImGuiDockSlot_Right:
        return ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20),
                      ImVec2(parent_rect.Max.x - 10, center.y + 20));
      default:
        IM_ASSERT(false);
    }
    IM_ASSERT(false);
    return ImRect();
  }

  Dock* getRootDock() {
    for (int i = 0; i < m_docks.size(); ++i) {
      if (!m_docks[i]->parent &&
          (m_docks[i]->status == Status_Docked || m_docks[i]->children[0])) {
        return m_docks[i];
      }
    }
    return nullptr;
  }

  bool dockSlots(Dock& dock,
                 Dock* dest_dock,
                 const ImRect& rect,
                 bool on_border) {
    ImDrawList* canvas = GetWindowDrawList();
    ImU32 color = GetColorU32(ImGuiCol_Button);
    ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);
    ImVec2 mouse_pos = GetIO().MousePos;
    for (int i = 0; i < (on_border ? 4 : 5); ++i) {
      ImRect r = on_border ? getSlotRectOnBorder(rect, (ImGuiDockSlot)i)
                           : getSlotRect(rect, (ImGuiDockSlot)i);
      bool hovered = r.Contains(mouse_pos);
      canvas->AddRectFilled(r.Min, r.Max, hovered ? color_hovered : color);
      if (!hovered)
        continue;

      if (!IsMouseDown(0)) {
        doDock(dock, dest_dock ? dest_dock : getRootDock(), (ImGuiDockSlot)i);
        return true;
      }
      ImRect docked_rect = getDockedRect(rect, (ImGuiDockSlot)i);
      canvas->AddRectFilled(
          docked_rect.Min, docked_rect.Max, GetColorU32(ImGuiCol_Button));
    }
    return false;
  }

  void handleDrag(Dock& dock) {
    Dock* dest_dock = getDockAt(GetIO().MousePos);

    Begin("##Overlay",
          NULL,
          ImVec2(0, 0),
          0.f,
          ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar |
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_NoSavedSettings |
              ImGuiWindowFlags_AlwaysAutoResize);
    ImDrawList* canvas = GetWindowDrawList();

    canvas->PushClipRectFullScreen();

    ImU32 docked_color = GetColorU32(ImGuiCol_FrameBg);
    docked_color = (docked_color & 0x00ffFFFF) | 0x80000000;
    dock.pos = GetIO().MousePos - m_drag_offset;
    if (dest_dock) {
      if (dockSlots(dock,
                    dest_dock,
                    ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size),
                    false)) {
        canvas->PopClipRect();
        End();
        return;
      }
    }
    if (dockSlots(dock,
                  nullptr,
                  ImRect(m_workspace_pos, m_workspace_pos + m_workspace_size),
                  true)) {
      canvas->PopClipRect();
      End();
      return;
    }
    canvas->AddRectFilled(dock.pos, dock.pos + dock.size, docked_color);
    canvas->PopClipRect();

    if (!IsMouseDown(0)) {
      dock.status = Status_Float;
      dock.location[0] = 0;
      dock.setActive();
    }

    End();
  }

  void fillLocation(Dock& dock) {
    if (dock.status == Status_Float)
      return;
    char* c = dock.location;
    Dock* tmp = &dock;
    while (tmp->parent) {
      *c = getLocationCode(tmp);
      tmp = tmp->parent;
      ++c;
    }
    *c = 0;
  }

  void doUndock(Dock& dock) {
    if (dock.prev_tab)
      dock.prev_tab->setActive();
    else if (dock.next_tab)
      dock.next_tab->setActive();
    else
      dock.active = false;
    Dock* container = dock.parent;

    if (container) {
      Dock& sibling = dock.getSibling();
      if (container->children[0] == &dock) {
        container->children[0] = dock.next_tab;
      } else if (container->children[1] == &dock) {
        container->children[1] = dock.next_tab;
      }

      bool remove_container =
          !container->children[0] || !container->children[1];
      if (remove_container) {
        if (container->parent) {
          Dock*& child = container->parent->children[0] == container
                             ? container->parent->children[0]
                             : container->parent->children[1];
          child = &sibling;
          child->setPosSize(container->pos, container->size);
          child->setParent(container->parent);
        } else {
          if (container->children[0]) {
            container->children[0]->setParent(nullptr);
            container->children[0]->setPosSize(container->pos, container->size);
          }
          if (container->children[1]) {
            container->children[1]->setParent(nullptr);
            container->children[1]->setPosSize(container->pos, container->size);
          }
        }
        for (int i = 0; i < m_docks.size(); ++i) {
          if (m_docks[i] == container) {
            m_docks.erase(m_docks.begin() + i);
            break;
          }
        }
        if (container == m_next_parent)
          m_next_parent = nullptr;
        container->~Dock();
        MemFree(container);
      }
    }
    if (dock.prev_tab)
      dock.prev_tab->next_tab = dock.next_tab;
    if (dock.next_tab)
      dock.next_tab->prev_tab = dock.prev_tab;
    dock.parent = nullptr;
    dock.prev_tab = dock.next_tab = nullptr;
  }

  void drawTabbarListButton(Dock& dock) {
    if (!dock.next_tab)
      return;

    ImDrawList* draw_list = GetWindowDrawList();
    if (InvisibleButton("list", ImVec2(16, 16))) {
      OpenPopup("tab_list_popup");
    }
    if (BeginPopup("tab_list_popup")) {
      Dock* tmp = &dock;
      while (tmp) {
        bool dummy = false;
        if (Selectable(tmp->label, &dummy)) {
          tmp->setActive();
          m_next_parent = tmp;
        }
        tmp = tmp->next_tab;
      }
      EndPopup();
    }

    bool hovered = IsItemHovered();
    ImVec2 min = GetItemRectMin();
    ImVec2 max = GetItemRectMax();
    ImVec2 center = (min + max) * 0.5f;
    ImU32 text_color = GetColorU32(ImGuiCol_Text);
    ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
    draw_list->AddRectFilled(ImVec2(center.x - 4, min.y + 3),
                             ImVec2(center.x + 4, min.y + 5),
                             hovered ? color_active : text_color);
    draw_list->AddTriangleFilled(ImVec2(center.x - 4, min.y + 7),
                                 ImVec2(center.x + 4, min.y + 7),
                                 ImVec2(center.x, min.y + 12),
                                 hovered ? color_active : text_color);
  }

  bool tabbar(Dock& dock, bool close_button) {
    float tabbar_height = 2 * GetTextLineHeightWithSpacing();
    ImVec2 size(dock.size.x, tabbar_height);
    bool tab_closed = false;

    SetCursorScreenPos(dock.pos);
    char tmp[20];
    ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock.id);
    if (BeginChild(tmp, size, true)) {
      Dock* dock_tab = &dock;

      ImDrawList* draw_list = GetWindowDrawList();
      ImU32 color = GetColorU32(ImGuiCol_FrameBg);
      ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
      ImU32 color_hovered = GetColorU32(ImGuiCol_FrameBgHovered);
      ImU32 text_color = GetColorU32(ImGuiCol_Text);
      float line_height = GetTextLineHeightWithSpacing();
      float tab_base;

      drawTabbarListButton(dock);

      while (dock_tab) {
        SameLine(0, 15);

        const char* text_end = FindRenderedTextEnd(dock_tab->label);
        ImVec2 size(CalcTextSize(dock_tab->label, text_end).x, line_height);
        if (InvisibleButton(dock_tab->label, size)) {
          dock_tab->setActive();
          m_next_parent = dock_tab;
        }

        if (IsItemActive() && IsMouseDragging()) {
          m_drag_offset = GetMousePos() - dock_tab->pos;
          doUndock(*dock_tab);
          dock_tab->status = Status_Dragged;
        }

        bool hovered = IsItemHovered();
        ImVec2 pos = GetItemRectMin();
        if (dock_tab->active && close_button) {
          size.x += 16 + GetStyle().ItemSpacing.x;
          SameLine();
          tab_closed = InvisibleButton("close", ImVec2(16, 16));
          ImVec2 center = (GetItemRectMin() + GetItemRectMax()) * 0.5f;
          draw_list->AddLine(center + ImVec2(-3.5f, -3.5f),
                             center + ImVec2(3.5f, 3.5f),
                             text_color);
          draw_list->AddLine(center + ImVec2(3.5f, -3.5f),
                             center + ImVec2(-3.5f, 3.5f),
                             text_color);
        }
        tab_base = pos.y;
        draw_list->PathClear();
        draw_list->PathLineTo(pos + ImVec2(-15, size.y));
        draw_list->PathBezierCurveTo(pos + ImVec2(-10, size.y),
                                     pos + ImVec2(-5, 0),
                                     pos + ImVec2(0, 0),
                                     10);
        draw_list->PathLineTo(pos + ImVec2(size.x, 0));
        draw_list->PathBezierCurveTo(pos + ImVec2(size.x + 5, 0),
                                     pos + ImVec2(size.x + 10, size.y),
                                     pos + ImVec2(size.x + 15, size.y),
                                     10);
        draw_list->PathFill(hovered
                                ? color_hovered
                                : (dock_tab->active ? color_active : color));
        draw_list->AddText(
            pos + ImVec2(0, 1), text_color, dock_tab->label, text_end);

        dock_tab = dock_tab->next_tab;
      }
      ImVec2 cp(dock.pos.x, tab_base + line_height);
      draw_list->AddLine(cp, cp + ImVec2(dock.size.x, 0), color);
    }
    EndChild();
    return tab_closed;
  }

  static void setDockPosSize(Dock& dest,
                             Dock& dock,
                             ImGuiDockSlot dock_slot,
                             Dock& container) {
    IM_ASSERT(!dock.prev_tab && !dock.next_tab && !dock.children[0] &&
              !dock.children[1]);

    dest.pos = container.pos;
    dest.size = container.size;
    dock.pos = container.pos;
    dock.size = container.size;

    switch (dock_slot) {
      case ImGuiDockSlot_Bottom:
        dest.size.y *= 0.5f;
        dock.size.y *= 0.5f;
        dock.pos.y += dest.size.y;
        break;
      case ImGuiDockSlot_Right:
        dest.size.x *= 0.5f;
        dock.size.x *= 0.5f;
        dock.pos.x += dest.size.x;
        break;
      case ImGuiDockSlot_Left:
        dest.size.x *= 0.5f;
        dock.size.x *= 0.5f;
        dest.pos.x += dock.size.x;
        break;
      case ImGuiDockSlot_Top:
        dest.size.y *= 0.5f;
        dock.size.y *= 0.5f;
        dest.pos.y += dock.size.y;
        break;
      default:
        IM_ASSERT(false);
        break;
    }
    dest.setPosSize(dest.pos, dest.size);

    if (container.children[1]->pos.x < container.children[0]->pos.x ||
        container.children[1]->pos.y < container.children[0]->pos.y) {
      Dock* tmp = container.children[0];
      container.children[0] = container.children[1];
      container.children[1] = tmp;
    }
  }

  void doDock(Dock& dock, Dock* dest, ImGuiDockSlot dock_slot) {
    IM_ASSERT(!dock.parent);
    if (!dest) {
      dock.status = Status_Docked;
      dock.setPosSize(m_workspace_pos, m_workspace_size);
    } else if (dock_slot == ImGuiDockSlot_Tab) {
      Dock* tmp = dest;
      while (tmp->next_tab) {
        tmp = tmp->next_tab;
      }

      tmp->next_tab = &dock;
      dock.prev_tab = tmp;
      dock.size = tmp->size;
      dock.pos = tmp->pos;
      dock.parent = dest->parent;
      dock.status = Status_Docked;
    } else if (dock_slot == ImGuiDockSlot_None) {
      dock.status = Status_Float;
    } else {
      Dock* container = (Dock*)MemAlloc(sizeof(Dock));
      IM_PLACEMENT_NEW(container) Dock();
      m_docks.push_back(container);
      container->children[0] = &dest->getFirstTab();
      container->children[1] = &dock;
      container->next_tab = nullptr;
      container->prev_tab = nullptr;
      container->parent = dest->parent;
      container->size = dest->size;
      container->pos = dest->pos;
      container->status = Status_Docked;
      container->label = ImStrdup("");

      if (!dest->parent) {
      } else if (&dest->getFirstTab() == dest->parent->children[0]) {
        dest->parent->children[0] = container;
      } else {
        dest->parent->children[1] = container;
      }

      dest->setParent(container);
      dock.parent = container;
      dock.status = Status_Docked;

      setDockPosSize(*dest, dock, dock_slot, *container);
    }
    dock.setActive();
  }

  void rootDock(const ImVec2& pos, const ImVec2& size) {
    Dock* root = getRootDock();
    if (!root)
      return;

    ImVec2 min_size = root->getMinSize();
    ImVec2 requested_size = size;
    root->setPosSize(pos, ImMax(min_size, requested_size));
  }

  void setDockActive() {
    IM_ASSERT(m_current);
    if (m_current)
      m_current->setActive();
  }

  static ImGuiDockSlot getSlotFromLocationCode(char code) {
    switch (code) {
      case '1':
        return ImGuiDockSlot_Left;
      case '2':
        return ImGuiDockSlot_Top;
      case '3':
        return ImGuiDockSlot_Bottom;
      default:
        return ImGuiDockSlot_Right;
    }
  }

  static char getLocationCode(Dock* dock) {
    if (!dock)
      return '0';

    if (dock->parent->isHorizontal()) {
      if (dock->pos.x < dock->parent->children[0]->pos.x)
        return '1';
      if (dock->pos.x < dock->parent->children[1]->pos.x)
        return '1';
      return '0';
    } else {
      if (dock->pos.y < dock->parent->children[0]->pos.y)
        return '2';
      if (dock->pos.y < dock->parent->children[1]->pos.y)
        return '2';
      return '3';
    }
  }

  void tryDockToStoredLocation(Dock& dock) {
    if (dock.status == Status_Docked)
      return;
    if (dock.location[0] == 0)
      return;

    Dock* tmp = getRootDock();
    if (!tmp)
      return;

    Dock* prev = nullptr;
    char* c = dock.location + strlen(dock.location) - 1;
    while (c >= dock.location && tmp) {
      prev = tmp;
      tmp = *c == getLocationCode(tmp->children[0]) ? tmp->children[0]
                                                    : tmp->children[1];
      if (tmp)
        --c;
    }
    doDock(dock,
           tmp ? tmp : prev,
           tmp ? ImGuiDockSlot_Tab : getSlotFromLocationCode(*c));
  }

  bool begin(const char* label, bool* opened, ImGuiWindowFlags extra_flags) {
    ImGuiDockSlot next_slot = m_next_dock_slot;
    m_next_dock_slot = ImGuiDockSlot_Tab;
    Dock& dock = getDock(label, !opened || *opened);
    if (!dock.opened && (!opened || *opened))
      tryDockToStoredLocation(dock);
    dock.last_frame = ImGui::GetFrameCount();
    if (strcmp(dock.label, label) != 0) {
      MemFree(dock.label);
      dock.label = ImStrdup(label);
    }

    m_end_action = EndAction_None;

    bool prev_opened = dock.opened;
    bool first = dock.first;
    if (dock.first && opened)
      *opened = dock.opened;
    dock.first = false;
    if (opened && !*opened) {
      if (dock.status != Status_Float) {
        fillLocation(dock);
        doUndock(dock);
        dock.status = Status_Float;
      }
      dock.opened = false;
      return false;
    }
    dock.opened = true;

    checkNonexistent();

    if (first || (prev_opened != dock.opened)) {
      Dock* root = m_next_parent ? m_next_parent : getRootDock();
      if (root && (&dock != root) && !dock.parent) {
        doDock(dock, root, next_slot);
      }
      m_next_parent = &dock;
    }

    m_current = &dock;
    if (dock.status == Status_Dragged)
      handleDrag(dock);

    bool is_float = dock.status == Status_Float;

    if (is_float) {
      SetNextWindowPos(dock.pos);
      SetNextWindowSize(dock.size);
      bool ret = Begin(label,
                       opened,
                       dock.size,
                       -1.0f,
                       ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_ShowBorders | extra_flags);
      m_end_action = EndAction_End;
      dock.pos = GetWindowPos();
      dock.size = GetWindowSize();

      ImGuiContext& g = *GImGui;

      if (g.ActiveId == GetCurrentWindow()->MoveId && g.IO.MouseDown[0]) {
        m_drag_offset = GetMousePos() - dock.pos;
        doUndock(dock);
        dock.status = Status_Dragged;
      }
      return ret;
    }

    if (!dock.active && dock.status != Status_Dragged)
      return false;

    // beginPanel();

    m_end_action = EndAction_EndChild;

    splits();

    PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    float tabbar_height = GetTextLineHeightWithSpacing();
    if (tabbar(dock.getFirstTab(), opened != nullptr)) {
      fillLocation(dock);
      *opened = false;
    }
    ImVec2 pos = dock.pos;
    ImVec2 size = dock.size;
    pos.y += tabbar_height + GetStyle().WindowPadding.y;
    size.y -= tabbar_height + GetStyle().WindowPadding.y;

    SetCursorScreenPos(pos);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | extra_flags;
    bool ret = BeginChild(label, size, true, flags);
    PopStyleColor();

    return ret;
  }

  void end() {
    m_current = nullptr;
    if (m_end_action != EndAction_None) {
      if (m_end_action == EndAction_End) {
        End();
      } else if (m_end_action == EndAction_EndChild) {
        PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        EndChild();
        PopStyleColor();
      }
      // endPanel();
    }
  }

  void debugWindow() {
    // SetNextWindowSize(ImVec2(300, 300));
    if (Begin("Dock Debug Info")) {
      for (int i = 0; i < m_docks.size(); ++i) {
        if (TreeNode(reinterpret_cast<void*>(static_cast<uintptr_t>(i)),
            "Dock %d (%p)",
            i,
            m_docks[i])) {
          Dock& dock = *m_docks[i];
          Text("pos=(%.1f %.1f) size=(%.1f %.1f)",
               dock.pos.x,
               dock.pos.y,
               dock.size.x,
               dock.size.y);
          Text("parent = %p\n", dock.parent);
          Text("isContainer() == %s\n", dock.isContainer() ? "true" : "false");
          Text("status = %s\n",
               (dock.status == Status_Docked)
                   ? "Docked"
                   : ((dock.status == Status_Dragged)
                          ? "Dragged"
                          : ((dock.status == Status_Float) ? "Float" : "?")));
          TreePop();
        }
      }
    }
    End();
  }

  int getDockIndex(Dock* dock) {
    if (!dock)
      return -1;

    for (int i = 0; i < m_docks.size(); ++i) {
      if (dock == m_docks[i])
        return i;
    }

    IM_ASSERT(false);
    return -1;
  }
};

static DockContext g_dock;


void ImGui::ShutdownDock()
{
	for (int i = 0; i < g_dock.m_docks.size(); ++i)
	{
		g_dock.m_docks[i]->~Dock();
		MemFree(g_dock.m_docks[i]);
	}
	g_dock.m_docks.clear();
}

void ImGui::SetNextDock(ImGuiDockSlot slot) {
    g_dock.m_next_dock_slot = slot;
}

void ImGui::BeginDockspace() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    BeginChild("###workspace", ImVec2(0,0), false, flags);
    g_dock.m_workspace_pos = GetWindowPos();
    g_dock.m_workspace_size = GetWindowSize();
}

void ImGui::EndDockspace() {
    EndChild();
}

void ImGui::SetDockActive()
{
	g_dock.setDockActive();
}


bool ImGui::BeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags)
{
	return g_dock.begin(label, opened, extra_flags);
}


void ImGui::EndDock()
{
	g_dock.end();
}

void ImGui::DockDebugWindow()
{
    g_dock.debugWindow();
}

/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////
/////////////////////////////// dock //////////////////////////////////////////

static void error_callback(int error, const char* description) {
  fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int, char**) {
  // Setup window
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    return 1;
  GLFWwindow* window =
      glfwCreateWindow(1280, 720, "ImGui OpenGL2 example", NULL, NULL);
  glfwMakeContextCurrent(window);

  // Setup ImGui binding
  ImGui_ImplGlfw_Init(window, true);

  // Load Fonts
  // (there is a default font, this is only if you want to change it. see
  // extra_fonts/README.txt for more details)
  // ImGuiIO& io = ImGui::GetIO();
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf",
  // 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // NULL, io.Fonts->GetGlyphRangesJapanese());

  bool show_test_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImColor(114, 144, 154);

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplGlfw_NewFrame();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in
    // a window automatically called "Debug"
    {
      static float f = 0.0f;
      ImGui::Text("Hello, world!");
      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
      ImGui::ColorEdit3("clear color", (float*)&clear_color);
      if (ImGui::Button("Test Window"))
        show_test_window ^= 1;
      if (ImGui::Button("Another Window"))
        show_another_window ^= 1;
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window) {
      ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
      ImGui::Begin("Another Window", &show_another_window);
      ImGui::Text("Hello");
      ImGui::End();
    }

    {
      ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiSetCond_FirstUseEver);
      if (ImGui::Begin("Dock Demo")) {
        ImGui::BeginDockspace();
        if (ImGui::BeginDock("Dock 1")) {
          ImGui::Text("This was a triumph...");
        }
        ImGui::EndDock();

        if (ImGui::BeginDock("Dock 2")) {
          ImGui::Text("I'm making a note here: huge success!");
        }
        ImGui::EndDock();

        if (ImGui::BeginDock("Dock 3")) {
          ImGui::Text("The cake is great!");
        }
        ImGui::EndDock();
        ImGui::EndDockspace();
      }
      ImGui::End();
    }

    // Rendering
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplGlfw_Shutdown();
  glfwTerminate();

  return 0;
}
