#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
把工具栏代码装进 LibreOffice 源码。不依赖行号，按内容精确替换，
所以官方源码更新也能正常应用。
"""
import os
import shutil
import sys

SRC = sys.argv[1] if len(sys.argv) > 1 else "/cygdrive/d/lo"
HERE = os.path.dirname(os.path.abspath(__file__))
SD = os.path.join(SRC, "sd")
SS = os.path.join(SD, "source", "ui", "slideshow")

def rep(path, old, new, label):
    with open(path, encoding="utf-8") as f:
        txt = f.read()
    if new in txt:
        print(f"  跳过（已存在）: {label}")
        return True
    if old not in txt:
        print(f"  找不到锚点: {label}")
        return False
    txt = txt.replace(old, new, 1)
    with open(path, "w", encoding="utf-8") as f:
        f.write(txt)
    print(f"  已应用: {label}")
    return True

def main():
    print("== 1. 复制新增文件 ==")
    for fn in ("showtoolbar.hxx", "showtoolbar.cxx"):
        shutil.copy(os.path.join(HERE, "patch", fn), os.path.join(SS, fn))
        print(f"  复制 {fn}")

    ok = True

    print("== 2. showwindow.hxx ==")
    hxx = os.path.join(SS, "showwindow.hxx")
    ok &= rep(hxx,
        "#include <Window.hxx>\n",
        '#include <Window.hxx>\n\n#include <memory>\n\n#include "showtoolbar.hxx"\n',
        "include")
    ok &= rep(hxx,
        "    void            DeleteWindowFromPaintView();",
        "    void            handleToolbarClick( const Point& rPos );\n\n"
        "    void            DeleteWindowFromPaintView();",
        "声明 handleToolbarClick")
    ok &= rep(hxx,
        "    ::rtl::Reference< SlideshowImpl > mxController;\n};",
        "    ::rtl::Reference< SlideshowImpl > mxController;\n\n"
        "    /** 放映时底部浮出的内置工具栏 */\n"
        "    std::unique_ptr< ShowToolbar > mpToolbar;\n};",
        "成员 mpToolbar")

    print("== 3. showwin.cxx ==")
    cxx = os.path.join(SS, "showwin.cxx")
    ok &= rep(cxx,
        '#include "showwindow.hxx"',
        '#include "showwindow.hxx"\n#include "showtoolbar.hxx"',
        "include")
    ok &= rep(cxx,
        "    maPauseTimer.SetInvokeHandler( LINK( this, ShowWindow, PauseTimeoutHdl ) );",
        "    mpToolbar = std::make_unique< ShowToolbar >();\n\n"
        "    maPauseTimer.SetInvokeHandler( LINK( this, ShowWindow, PauseTimeoutHdl ) );",
        "构造工具栏")
    ok &= rep(cxx,
        "void ShowWindow::Paint(vcl::RenderContext& /*rRenderContext*/, const ::tools::Rectangle& rRect)\n"
        "{\n"
        "    if( (meShowWindowMode == SHOWWINDOWMODE_NORMAL) || (meShowWindowMode == SHOWWINDOWMODE_PREVIEW) )\n"
        "    {\n"
        "        if( mxController.is() )\n"
        "        {\n"
        "            mxController->paint();\n"
        "        }\n"
        "        else if(mpViewShell )\n"
        "        {\n"
        "            mpViewShell->Paint(rRect, this);\n"
        "        }\n"
        "    }",
        "void ShowWindow::Paint(vcl::RenderContext& rRenderContext, const ::tools::Rectangle& rRect)\n"
        "{\n"
        "    if( (meShowWindowMode == SHOWWINDOWMODE_NORMAL) || (meShowWindowMode == SHOWWINDOWMODE_PREVIEW) )\n"
        "    {\n"
        "        if( mxController.is() )\n"
        "        {\n"
        "            mxController->paint();\n"
        "        }\n"
        "        else if(mpViewShell )\n"
        "        {\n"
        "            mpViewShell->Paint(rRect, this);\n"
        "        }\n"
        "\n"
        "        // 内置工具栏：叠加绘制在放映画面上\n"
        "        if( mpToolbar && SHOWWINDOWMODE_NORMAL == meShowWindowMode && mxController.is() )\n"
        "        {\n"
        "            const sal_Int32 nIndex = mxController->getCurrentSlideIndex();\n"
        "            mpToolbar->setPageInfo( nIndex + 1, mxController->getCount() );\n"
        "            mpToolbar->paint( rRenderContext, GetOutputSizePixel() );\n"
        "        }\n"
        "    }",
        "Paint 叠加绘制")
    ok &= rep(cxx,
        "void ShowWindow::LoseFocus()",
        "void ShowWindow::handleToolbarClick( const Point& rPos )\n"
        "{\n"
        "    if( !mxController.is() || !mpToolbar )\n"
        "        return;\n"
        "\n"
        "    switch( mpToolbar->handleClick( rPos, GetOutputSizePixel() ) )\n"
        "    {\n"
        "        case ShowToolbar::Action::PrevPage:\n"
        "            mxController->gotoPreviousSlide();\n"
        "            break;\n"
        "        case ShowToolbar::Action::NextPage:\n"
        "            mxController->gotoNextSlide();\n"
        "            break;\n"
        "        case ShowToolbar::Action::OpenLeftSidebar:\n"
        "            mpToolbar->toggleSidebar( true );\n"
        "            break;\n"
        "        case ShowToolbar::Action::OpenRightSidebar:\n"
        "            mpToolbar->toggleSidebar( false );\n"
        "            break;\n"
        "        case ShowToolbar::Action::Pen:\n"
        "            mpToolbar->setTool( ShowToolbar::Tool::Pen );\n"
        "            break;\n"
        "        case ShowToolbar::Action::Eraser:\n"
        "            mpToolbar->setTool( ShowToolbar::Tool::Eraser );\n"
        "            break;\n"
        "        case ShowToolbar::Action::Select:\n"
        "            mpToolbar->setTool( ShowToolbar::Tool::Select );\n"
        "            break;\n"
        "        case ShowToolbar::Action::EndShow:\n"
        "            mxController->endPresentation();\n"
        "            return;\n"
        "        default:\n"
        "            break;\n"
        "    }\n"
        "    Invalidate();\n"
        "}\n"
        "\n"
        "void ShowWindow::LoseFocus()",
        "handleToolbarClick 实现")

    print("== 4. Library_sd.mk ==")
    mk = os.path.join(SD, "Library_sd.mk")
    ok &= rep(mk,
        "\tsd/source/ui/slideshow/showwin \\",
        "\tsd/source/ui/slideshow/showwin \\\n\tsd/source/ui/slideshow/showtoolbar \\",
        "加入编译列表")

    print("== 完成 ==")
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
