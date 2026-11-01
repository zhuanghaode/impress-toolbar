# Impress 放映工具栏 —— Windows 云端编译

在 GitHub 免费 Windows 虚拟机上，把官方 LibreOffice 源码编译成带底部放映工具栏的 Windows 版本。

## 这个仓库里有什么

| 文件 | 作用 |
|---|---|
| `impress-toolbar.patch` | 全部代码改动（478 行），会自动应用到官方源码上 |
| `.github/workflows/build.yml` | 云编译脚本 |

**不包含** LibreOffice 源码本身。编译时云端会自己下载官方源码，然后打上补丁，所以仓库很小。

## 改了哪些文件

| 文件 | 改动 |
|---|---|
| `sd/source/ui/slideshow/showtoolbar.hxx` | 新增：工具栏类 |
| `sd/source/ui/slideshow/showtoolbar.cxx` | 新增：绘制与点击实现 |
| `sd/source/ui/slideshow/showwin.cxx` | 修改：在 `Paint()` 里叠加绘制、拦截工具栏点击 |
| `sd/source/ui/slideshow/showwindow.hxx` | 修改：增加工具栏成员 |
| `sd/Library_sd.mk` | 修改：加入编译列表 |

## 效果

放映时的画面底部会多出一条工具栏：

```
[←][2/40][→]        [笔][橡皮][撤销][重做][清屏][选择][结束放映]        [←][2/40][→]
```

- 默认亮色
- 三段分开，左右两组与中间一组之间留有大空隙
- 图标在上、中文文字在下

## 怎么用

1. 在 GitHub 上新建一个**私有仓库**
2. 把本目录的文件推上去（包含 `.github` 目录）
3. 仓库页面 → **Actions** → 左侧选 **Build Impress with Toolbar (Windows x64)**
4. 点右侧 **Run workflow** → 再点绿色按钮
5. 等待编译（约 1~3 小时）
6. 完成后，在该次运行页面底部 **Artifacts** 里下载 `libreoffice-impress-toolbar-win64.zip`
7. 解压后运行 `instdir/program/soffice.exe`

## 注意

- 编译成功不等于效果一定正确。这台云机器没有图形界面，**看不到放映时的实际画面**，需要安装后由人试用确认。
- 如果编译失败，把 Actions 页面里的报错贴出来，修补丁后重新运行即可。
- 补丁基于 `libreoffice-26-8` 分支。如果官方源码更新导致补丁冲突，需要调整补丁。
