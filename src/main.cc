// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#if 0
#include "core.h"
#include "dbgeng/debugger_dbgeng.h"
#include "debugger.h"
#include "docking_split_container.h"
#include "docking_tool_window.h"
#include "docking_workspace.h"
#include "entry.h"
#include "focus.h"
#include "gfx.h"
#include "skin.h"
#include "solid_color.h"
#include "source_view/source_view.h"
#include "text_edit.h"
#include "tree_grid.h"

#if 0
#include "clang-c/Index.h"
#endif

void FillColumns(TreeGridNode* node,
                 const char* name,
                 const char* value,
                 const char* type) {
  node->SetValue(0, new TreeGridNodeValueString(name));
  node->SetValue(1, new TreeGridNodeValueString(value));
  node->SetValue(2, new TreeGridNodeValueString(type));
}

void FillWatchWithSampleData(TreeGrid* watch) {
  // The TreeGrid owns all these pointers once they're added.

  TreeGridColumn* name_column = new TreeGridColumn(watch, "Name");
  TreeGridColumn* value_column = new TreeGridColumn(watch, "Value");
  TreeGridColumn* type_column = new TreeGridColumn(watch, "Type");
  watch->Columns()->push_back(name_column);
  watch->Columns()->push_back(value_column);
  watch->Columns()->push_back(type_column);
  name_column->SetWidthPercentage(0.25f);
  value_column->SetWidthPercentage(0.5f);
  type_column->SetWidthPercentage(0.25f);

  TreeGridNode* root0 = new TreeGridNode(watch, NULL);
  watch->Nodes()->push_back(root0);
  FillColumns(root0,
              "this",
              "{root_=unique_ptr {direction_=kSplitNoneRoot (0) "
              "fraction_=0.50000000000000000 left_=unique_ptr "
              "{direction_=kSplitHorizontal (2) fraction_=0.69999999999999996 "
              "left_=unique_ptr ...} ...} ...}",
              "DockingWorkspace *");
  root0->SetExpanded(true);

  TreeGridNode* child0 = new TreeGridNode(watch, root0);
  root0->Nodes()->push_back(child0);
  FillColumns(child0, "InputHandler", "{...}", "InputHandler");

  TreeGridNode* child1 = new TreeGridNode(watch, root0);
  root0->Nodes()->push_back(child1);
  FillColumns(child1,
              "root_",
              "unique_ptr {direction_=kSplitNoneRoot (0) "
              "fraction_=0.50000000000000000 left_=unique_ptr "
              "{direction_=kSplitHorizontal (2) fraction_=0.69999999999999996 "
              "left_=unique_ptr {...} ...} ...}",
              "std::unique_ptr<DockingSplitContainer,std::default_delete<"
              "DockingSplitContainer> >");

  TreeGridNode* child2 = new TreeGridNode(watch, root0);
  root0->Nodes()->push_back(child2);
  FillColumns(child2, "mouse_position_", "{x=1924 y=440 }", "Point");

  TreeGridNode* child2_0 = new TreeGridNode(watch, child2);
  child2->Nodes()->push_back(child2_0);
  FillColumns(child2_0, "x", "1924", "int");

  TreeGridNode* child2_1 = new TreeGridNode(watch, child2);
  child2->Nodes()->push_back(child2_1);
  FillColumns(child2_1, "y", "440", "int");

  TreeGridNode* child3 = new TreeGridNode(watch, root0);
  root0->Nodes()->push_back(child3);
  FillColumns(child3,
              "draggable_",
              "empty",
              "std::unique_ptr<Draggable,std::default_delete<Draggable> >");

  TreeGridNode* root1 = new TreeGridNode(watch, NULL);
  watch->Nodes()->push_back(root1);
  FillColumns(root1,
              "target",
              "0x040a87b0 {color_={rgba=0x040a87c8 {0.000000000, 0.168627456, "
              "0.211764708, 1.00000000} r=0.000000000 ...} }",
              "Dockable *");
}

class MainLoop : public DebuggerDelegate {
 public:
  MainLoop() {
    //scoped_ptr<Debugger> debugger(new DebuggerDbgEng);
    //debugger->LaunchProcess("scratch/test_target.exe");

    const Skin& skin = Skin::current();
    const ColorScheme& cs = skin.GetColorScheme();
    SourceView* source_view = new SourceView();
    source_view->SetFilePath("src\\main.cc");

    DockingToolWindow* stack =
        new DockingToolWindow(new SolidColor(cs.background()), "Stack");

    TreeGrid* watch_contents = new TreeGrid();
    DockingToolWindow* watch = new DockingToolWindow(watch_contents, "Watch");

    FillWatchWithSampleData(watch_contents);
    // TODO(scottmg): Figure out who's responsible for dealing on mutate.
    watch_contents->MoveFocusByDirection(TreeGrid::kFocusDown);

    DockingToolWindow* breakpoints =
        new DockingToolWindow(new TreeGrid(), "Breakpoints");

    TextEdit* command_contents = new TextEdit();
    DockingToolWindow* command =
        new DockingToolWindow(command_contents, "Command");
    SetFocusedContents(command_contents);

    main_area_.SetRoot(source_view);
    source_view->parent()->AsDockingSplitContainer()->SplitChild(
        kSplitHorizontal, source_view, command);
    source_view->parent()->AsDockingSplitContainer()->SetFraction(0.7f);

    source_view->parent()->AsDockingSplitContainer()->SplitChild(
        kSplitVertical, source_view, stack);
    source_view->parent()->AsDockingSplitContainer()->SetFraction(0.4f);

    stack->parent()->AsDockingSplitContainer()->SplitChild(
        kSplitVertical, stack, watch);
    stack->parent()->AsDockingSplitContainer()->SetFraction(0.5f);

    stack->parent()->AsDockingSplitContainer()->SplitChild(
        kSplitHorizontal, stack, breakpoints);
    stack->parent()->AsDockingSplitContainer()->SetFraction(0.7f);
  }

  void Run() {
    const Skin& skin = Skin::current();

    uint32_t prev_width = 0, prev_height = 0;
    uint32_t width, height;
    while (!ProcessEvents(&width, &height, &main_area_)) {
      if (prev_width != width || prev_height != height) {
        main_area_.SetScreenRect(
            Rect(skin.border_size() / GetDpiScale(),
                skin.border_size() / GetDpiScale(),
                (width - skin.border_size() * 2) / GetDpiScale(),
                (height - skin.border_size() * 2) / GetDpiScale()));
        GfxResize(width, height);
        prev_width = width;
        prev_height = height;
      }

      main_area_.Render();
      GfxDrawFps();
      GfxFrame();
    }
  }

 private:

  // DebuggerDelegate:
  void OnProcessLaunched(ProcessId process_id) {
    printf("Launched(%lld)\n", process_id);
  }

  void OnStopped(ProcessId process_id) {
    printf("Stopped(%lld)\n", process_id);
  }

  void OnGotLocation(ProcessId process_id, uint64_t ip) {
    printf("Location(%lld): %lld\n", process_id, ip);
  }



  DockingWorkspace main_area_;
  //scoped_ptr<Debugger> debugger_;
  
  DISALLOW_COPY_AND_ASSIGN(MainLoop);
};

int Main(int argc, char** argv) {
  GfxInit();
  Skin::LoadData();

  UNUSED(argc);
  UNUSED(argv);

  MainLoop main_loop;
  main_loop.Run();

  GfxShutdown();

  return 0;
}
#endif
int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  return 0;
}
