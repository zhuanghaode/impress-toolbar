/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "showtoolbar.hxx"

#include <tools/fontenum.hxx>
#include <vcl/outdev.hxx>
#include <vcl/virdev.hxx>
#include <vcl/settings.hxx>

namespace sd
{

namespace
{
// 逻辑尺寸（单位：100th mm 之外的像素，按窗口实际像素缩放）
constexpr sal_Int32 BAR_HEIGHT = 76;
constexpr sal_Int32 BAR_MARGIN = 18;

constexpr Color COL_BG( 0xFA, 0xFB, 0xFD );       // 亮色底
constexpr Color COL_FG( 0x1F, 0x25, 0x30 );
constexpr Color COL_DIM( 0x5B, 0x66, 0x76 );
constexpr Color COL_ACCENT( 0x1A, 0x73, 0xE8 );
constexpr Color COL_BORDER( 0xD5, 0xDB, 0xE5 );

constexpr Color COL_HOVER( 0xEC, 0xF1, 0xF8 );

/** 按钮下方的文字标签。用转义写法，保证任何编译器/编码下都正确。 */
OUString labelForIcon( sal_Int32 nIcon )
{
    switch( nIcon )
    {
        case 3: return u"\u7B14"_ustr;              // 笔
        case 4: return u"\u6A61\u76AE"_ustr;       // 橡皮
        case 5: return u"\u64A4\u9500"_ustr;       // 撤销
        case 6: return u"\u91CD\u505A"_ustr;       // 重做
        case 7: return u"\u6E05\u5C4F"_ustr;       // 清屏
        case 8: return u"\u9009\u62E9"_ustr;       // 选择
        case 9: return u"\u7ED3\u675F\u653E\u6620"_ustr;  // 结束放映
        default: return OUString();
    }
}
}

ShowToolbar::ShowToolbar()
    : mnCurrent( 1 )
    , mnTotal( 1 )
    , meTool( Tool::Select )
    , mbSidebarVisible( false )
    , mbSidebarLeft( true )
    , mbLayoutDone( false )
    , maBgColor( COL_BG )
    , maFgColor( COL_FG )
    , maDimColor( COL_DIM )
    , maAccentColor( COL_ACCENT )
    , maBorderColor( COL_BORDER )
{
}

ShowToolbar::~ShowToolbar() = default;

void ShowToolbar::setPageInfo( sal_Int32 nCurrent, sal_Int32 nTotal )
{
    mnCurrent = nCurrent;
    mnTotal = nTotal;
}

void ShowToolbar::layoutButtons( const Size& rWindowSize )
{
    // 按缩放系数取整
    const sal_Int32 nBarH = BAR_HEIGHT;
    const sal_Int32 nMargin = BAR_MARGIN;
    const sal_Int32 nBtnW = 96;
    const sal_Int32 nNavW = 58;
    const sal_Int32 nPageW = 132;
    const sal_Int32 nGap = 4;

    auto makeRow = [&]( sal_Int32 nCenterX, std::vector<Button>& rBuf ) -> ::tools::Rectangle
    {
        sal_Int32 nTotalW = nNavW + nPageW + nNavW + nGap * 2;
        sal_Int32 nX = nCenterX - nTotalW / 2;
        sal_Int32 nY = rWindowSize.Height() - nBarH - nMargin + 8;
        ::tools::Rectangle aZone( nX, nY, nX + nTotalW, nY + nBarH - 16 );

        rBuf.push_back( { ::tools::Rectangle( nX, nY, nX + nNavW, nY + nBarH - 16 ),
                          Action::PrevPage, OUString(), 0 } );
        nX += nNavW + nGap;
        // 页码按钮：左右半边各算一个热区，这里给整块，点击时再分半边
        rBuf.push_back( { ::tools::Rectangle( nX, nY, nX + nPageW, nY + nBarH - 16 ),
                          Action::OpenLeftSidebar, OUString(), 1 } );
        nX += nPageW + nGap;
        rBuf.push_back( { ::tools::Rectangle( nX, nY, nX + nNavW, nY + nBarH - 16 ),
                          Action::NextPage, OUString(), 2 } );
        return aZone;
    };

    maButtons.clear();
    const sal_Int32 nZoneCenterLeft = nMargin + ( nNavW + nPageW + nNavW + nGap * 2 ) / 2;
    maLeftZone = makeRow( nZoneCenterLeft, maButtons );

    // 右组对称
    const sal_Int32 nZoneCenterRight =
        rWindowSize.Width() - nMargin - ( nNavW + nPageW + nNavW + nGap * 2 ) / 2;
    std::vector<Button> aRightButtons;
    maRightZone = makeRow( nZoneCenterRight, aRightButtons );
    for( auto& rButton : aRightButtons )
    {
        if( rButton.eAction == Action::OpenLeftSidebar )
            rButton.eAction = Action::OpenRightSidebar;
        maButtons.push_back( rButton );
    }

    // 中间一坨：七个按钮
    {
        static const struct { Action eAction; sal_Int32 nIcon; } aMid[] = {
            { Action::Pen, 3 }, { Action::Eraser, 4 }, { Action::Undo, 5 },
            { Action::Redo, 6 }, { Action::Clear, 7 }, { Action::Select, 8 },
            { Action::EndShow, 9 }
        };
        const sal_Int32 nCount = SAL_N_ELEMENTS( aMid );
        const sal_Int32 nMidW = nBtnW * nCount + nGap * ( nCount - 1 );
        sal_Int32 nX = ( rWindowSize.Width() - nMidW ) / 2;
        const sal_Int32 nY = rWindowSize.Height() - nBarH - nMargin + 8;
        ::tools::Rectangle aZone( nX, nY, nX + nMidW, nY + nBarH - 16 );
        for( sal_Int32 i = 0; i < nCount; ++i )
        {
            maButtons.push_back( { ::tools::Rectangle( nX, nY, nX + nBtnW, nY + nBarH - 16 ),
                                   aMid[i].eAction, OUString(), aMid[i].nIcon } );
            nX += nBtnW + nGap;
        }
        maMidZone = aZone;
    }

    maBarRect = ::tools::Rectangle( nMargin, rWindowSize.Height() - nBarH - nMargin,
                                  rWindowSize.Width() - nMargin, rWindowSize.Height() - nMargin );
    mbLayoutDone = true;
}

void ShowToolbar::paintButton( OutputDevice& rOutDev, const Button& rButton, bool bActive )
{
    rOutDev.SetLineColor();
    rOutDev.SetFillColor( bActive ? Color( 0xE3, 0xEF, 0xFD ) : COL_HOVER );
    rOutDev.DrawRect( rButton.aRect );
}

void ShowToolbar::paintPageButton( OutputDevice& rOutDev, const Button& rButton )
{
    // 页码：2 / 40
    OUString aText = OUString::number( mnCurrent ) + u"/"_ustr + OUString::number( mnTotal );
    vcl::Font aFont = rOutDev.GetFont();
    aFont.SetFontSize( Size( 0, 15 ) );
    aFont.SetWeight( WEIGHT_BOLD );
    aFont.SetColor( maFgColor );
    rOutDev.SetFont( aFont );
    rOutDev.DrawText( rButton.aRect, aText, DrawTextFlags::Center | DrawTextFlags::VCenter );
}

void ShowToolbar::paintGlyph( OutputDevice& rOutDev, sal_Int32 nIcon,
                              const ::tools::Rectangle& rBox, const Color& rColor )
{
    // 用基础图元画图标，不依赖外部图标资源
    rOutDev.SetLineColor( rColor );
    rOutDev.SetFillColor( rColor );
    const Point aC = rBox.Center();
    const sal_Int32 nS = std::min( rBox.GetWidth(), rBox.GetHeight() ) / 2;

    switch( nIcon )
    {
        case 0:   // 左箭头
            rOutDev.DrawLine( Point( aC.X() + nS / 2, aC.Y() - nS / 2 ), Point( aC.X() - nS / 2, aC.Y() ) );
            rOutDev.DrawLine( Point( aC.X() - nS / 2, aC.Y() ), Point( aC.X() + nS / 2, aC.Y() + nS / 2 ) );
            break;
        case 2:   // 右箭头
            rOutDev.DrawLine( Point( aC.X() - nS / 2, aC.Y() - nS / 2 ), Point( aC.X() + nS / 2, aC.Y() ) );
            rOutDev.DrawLine( Point( aC.X() + nS / 2, aC.Y() ), Point( aC.X() - nS / 2, aC.Y() + nS / 2 ) );
            break;
        case 3:   // 笔
            rOutDev.DrawLine( Point( aC.X() - nS / 2, aC.Y() + nS / 2 ), Point( aC.X() + nS / 2, aC.Y() - nS / 2 ) );
            rOutDev.DrawLine( Point( aC.X() + nS / 4, aC.Y() - nS / 2 ), Point( aC.X() + nS / 2, aC.Y() - nS / 4 ) );
            break;
        case 4:   // 橡皮
            rOutDev.DrawRect( ::tools::Rectangle( aC.X() - nS / 2, aC.Y() - nS / 3,
                                                  aC.X() + nS / 2, aC.Y() + nS / 3 ) );
            rOutDev.DrawLine( Point( aC.X() - nS / 2, aC.Y() + nS / 3 ), Point( aC.X() + nS / 2, aC.Y() + nS / 3 ) );
            break;
        case 5:   // 撤销
        case 6:   // 重做
        {
            const bool bUndo = ( nIcon == 5 );
            rOutDev.DrawLine( Point( aC.X(), aC.Y() - nS / 2 ), Point( aC.X() + ( bUndo ? -nS / 2 : nS / 2 ), aC.Y() - nS / 2 ) );
            rOutDev.DrawLine( Point( aC.X() + ( bUndo ? -nS / 2 : nS / 2 ), aC.Y() - nS / 2 ),
                              Point( aC.X() + ( bUndo ? -nS / 2 : nS / 2 ), aC.Y() ) );
            rOutDev.DrawLine( Point( aC.X() + ( bUndo ? -nS / 2 : nS / 2 ), aC.Y() ), Point( aC.X(), aC.Y() ) );
            break;
        }
        case 7:   // 清屏
            rOutDev.DrawLine( Point( aC.X() - nS / 2, aC.Y() - nS / 3 ), Point( aC.X() + nS / 2, aC.Y() - nS / 3 ) );
            rOutDev.DrawLine( Point( aC.X() - nS / 3, aC.Y() - nS / 3 ), Point( aC.X() - nS / 3, aC.Y() + nS / 3 ) );
            rOutDev.DrawLine( Point( aC.X() + nS / 3, aC.Y() - nS / 3 ), Point( aC.X() + nS / 3, aC.Y() + nS / 3 ) );
            rOutDev.DrawLine( Point( aC.X() - nS / 3, aC.Y() + nS / 3 ), Point( aC.X() + nS / 3, aC.Y() + nS / 3 ) );
            break;
        case 8:   // 选择（鼠标指针）
            rOutDev.DrawLine( Point( aC.X() - nS / 3, aC.Y() - nS / 2 ), Point( aC.X() + nS / 2, aC.Y() ) );
            rOutDev.DrawLine( Point( aC.X() + nS / 2, aC.Y() ), Point( aC.X() - nS / 3, aC.Y() + nS / 2 ) );
            rOutDev.DrawLine( Point( aC.X() - nS / 3, aC.Y() + nS / 2 ), Point( aC.X() - nS / 3, aC.Y() - nS / 2 ) );
            break;
        case 9:   // 结束放映
            rOutDev.DrawRect( ::tools::Rectangle( aC.X() - nS / 2, aC.Y() - nS / 3,
                                                  aC.X() + nS / 2, aC.Y() + nS / 3 ) );
            rOutDev.SetFillColor( rColor );
            rOutDev.DrawRect( ::tools::Rectangle( aC.X() - nS / 5, aC.Y() - nS / 8,
                                                  aC.X() + nS / 5, aC.Y() + nS / 8 ) );
            break;
        default:
            break;
    }
}

::tools::Rectangle ShowToolbar::paint( OutputDevice& rOutDev, const Size& rWindowSize )
{
    layoutButtons( rWindowSize );

    // 背景：亮色圆角条
    rOutDev.SetLineColor( maBorderColor );
    rOutDev.SetFillColor( maBgColor );
    rOutDev.DrawRect( maBarRect );

    for( const auto& rButton : maButtons )
    {
        const bool bActive =
            ( rButton.eAction == Action::Pen && meTool == Tool::Pen )
            || ( rButton.eAction == Action::Select && meTool == Tool::Select );

        if( bActive )
            paintButton( rOutDev, rButton, true );

        if( rButton.nIcon == 1 )
        {
            paintPageButton( rOutDev, rButton );
        }
        else
        {
            // 图标画在上部，文字标签贴在下部
            ::tools::Rectangle aIconBox = rButton.aRect;
            const sal_Int32 nH = aIconBox.GetHeight();
            aIconBox.SetBottom( aIconBox.Top() + nH * 2 / 3 );
            paintGlyph( rOutDev, rButton.nIcon, aIconBox,
                        bActive ? maAccentColor : maFgColor );

            const OUString aLabel = labelForIcon( rButton.nIcon );
            if( !aLabel.isEmpty() )
            {
                vcl::Font aFont = rOutDev.GetFont();
                aFont.SetFontSize( Size( 0, 10 ) );
                aFont.SetWeight( WEIGHT_NORMAL );
                aFont.SetColor( bActive ? maAccentColor : maDimColor );
                rOutDev.SetFont( aFont );

                ::tools::Rectangle aTextRect = rButton.aRect;
                aTextRect.SetTop( aIconBox.Top() + nH * 2 / 3 );
                rOutDev.DrawText( aTextRect, aLabel,
                                  DrawTextFlags::Center | DrawTextFlags::VCenter );
            }
        }
    }

    return maBarRect;
}

bool ShowToolbar::hitTest( const Point& rPos, const Size& rWindowSize ) const
{
    if( !mbLayoutDone )
        const_cast<ShowToolbar*>( this )->layoutButtons( rWindowSize );

    for( const auto& rButton : maButtons )
    {
        if( rButton.aRect.Contains( rPos ) )
            return true;
    }
    return false;
}

ShowToolbar::Action ShowToolbar::handleClick( const Point& rPos, const Size& rWindowSize )
{
    if( !mbLayoutDone )
        layoutButtons( rWindowSize );

    for( const auto& rButton : maButtons )
    {
        if( !rButton.aRect.Contains( rPos ) )
            continue;

        // 页码按钮分左右半边
        if( rButton.eAction == Action::OpenLeftSidebar
            || rButton.eAction == Action::OpenRightSidebar )
        {
            const sal_Int32 nMid = rButton.aRect.Center().X();
            return ( rPos.X() < nMid ) ? Action::OpenLeftSidebar : Action::OpenRightSidebar;
        }
        return rButton.eAction;
    }
    return Action::None;
}

} // end of namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
