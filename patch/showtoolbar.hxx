/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SD_SOURCE_UI_SLIDESHOW_SHOWTOOLBAR_HXX
#define INCLUDED_SD_SOURCE_UI_SLIDESHOW_SHOWTOOLBAR_HXX

#include <vector>

#include <tools/color.hxx>
#include <tools/gen.hxx>

class Point;
class MouseEvent;
class OutputDevice;

namespace sd
{

/** 放映时底部浮出的工具栏（内置在放映窗口里绘制）。

    三段式布局，左右两段与中间一坨之间留出明显空隙：
      左 [←][2/40][→]      中 [笔][橡皮][撤销][重做][清屏][选择][结束放映]      右 [←][2/40][→]
 */
class ShowToolbar
{
public:
    enum class Tool { Select, Pen, Eraser };

    ShowToolbar();
    ~ShowToolbar();

    void setPageInfo( sal_Int32 nCurrent, sal_Int32 nTotal );
    sal_Int32 getCurrent() const { return mnCurrent; }
    sal_Int32 getTotal() const { return mnTotal; }

    Tool getTool() const { return meTool; }
    void setTool( Tool eTool ) { meTool = eTool; }

    /** 在放映窗口上绘制工具栏。返回被工具栏占用的区域（屏幕像素）。 */
    ::tools::Rectangle paint( OutputDevice& rOutDev, const Size& rWindowSize );

    /** 工具栏是否吃掉这个鼠标事件。 */
    bool hitTest( const Point& rPos, const Size& rWindowSize ) const;

    /** 处理点击；返回发生的事件类型。 */
    enum class Action
    {
        None,
        PrevPage,
        NextPage,
        OpenLeftSidebar,
        OpenRightSidebar,
        Pen,
        Eraser,
        Undo,
        Redo,
        Clear,
        Select,
        EndShow
    };
    Action handleClick( const Point& rPos, const Size& rWindowSize );

    /** 边栏状态 */
    bool isSidebarVisible() const { return mbSidebarVisible; }
    bool isSidebarOnLeft() const { return mbSidebarLeft; }
    void toggleSidebar( bool bLeft )
    {
        if( mbSidebarVisible && mbSidebarLeft == bLeft )
            mbSidebarVisible = false;   // 同一边再点一次 -> 收起
        else
        {
            mbSidebarVisible = true;
            mbSidebarLeft = bLeft;
        }
    }
    void hideSidebar() { mbSidebarVisible = false; }

private:
    struct Button
    {
        ::tools::Rectangle aRect;
        Action eAction;
        OUString aLabel;
        sal_Int32 nIcon;     // 图标索引
    };

    void layoutButtons( const Size& rWindowSize );
    void paintButton( OutputDevice& rOutDev, const Button& rButton, bool bActive );
    void paintGlyph( OutputDevice& rOutDev, sal_Int32 nIcon, const ::tools::Rectangle& rBox,
                     const Color& rColor );
    void paintPageButton( OutputDevice& rOutDev, const Button& rButton );

    std::vector<Button> maButtons;
    ::tools::Rectangle maBarRect;
    ::tools::Rectangle maLeftZone;
    ::tools::Rectangle maMidZone;
    ::tools::Rectangle maRightZone;

    sal_Int32 mnCurrent;
    sal_Int32 mnTotal;
    Tool meTool;
    bool mbSidebarVisible;
    bool mbSidebarLeft;
    bool mbLayoutDone;

    // 亮色主题（默认）
    Color maBgColor;        // 工具栏底色
    Color maFgColor;        // 图标/文字
    Color maDimColor;       // 次要文字
    Color maAccentColor;    // 选中态
    Color maBorderColor;
};

} // end of namespace sd

#endif // INCLUDED_SD_SOURCE_UI_SLIDESHOW_SHOWTOOLBAR_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
