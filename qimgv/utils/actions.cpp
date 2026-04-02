#include "actions.h"
#include <unordered_map>

Actions *appActions = nullptr;

Actions::Actions() {
    init();
}

Actions *Actions::getInstance() {
    if(!appActions)
        appActions  = new Actions();
    return appActions;
}

void Actions::init() {
    mActions = {
        {"nextImage", QVersionNumber(0,6,2)},
        {"prevImage", QVersionNumber(0,6,2)},
        {"toggleFullscreen", QVersionNumber(0,6,2)},
        {"fitWindow", QVersionNumber(0,6,2)},
        {"fitWidth", QVersionNumber(0,6,2)},
        {"fitNormal", QVersionNumber(0,6,2)},
        {"fitWindowStretch", QVersionNumber(1,0,3)},
        {"toggleFitMode", QVersionNumber(0,6,2)},
        {"resize", QVersionNumber(0,6,2)},
        {"rotateRight", QVersionNumber(0,6,2)},
        {"rotateLeft", QVersionNumber(0,6,2)},
        {"scrollUp", QVersionNumber(0,6,2)},
        {"scrollDown", QVersionNumber(0,6,2)},
        {"scrollLeft", QVersionNumber(0,6,2)},
        {"scrollRight", QVersionNumber(0,6,2)},
        {"zoomIn", QVersionNumber(0,6,2)},
        {"zoomOut", QVersionNumber(0,6,2)},
        {"zoomInCursor", QVersionNumber(0,6,2)},
        {"zoomOutCursor", QVersionNumber(0,6,2)},
        {"open", QVersionNumber(0,6,2)},
        {"save", QVersionNumber(0,6,2)},
        {"saveAs", QVersionNumber(0,6,2)},
        {"setWallpaper", QVersionNumber(0,9,3)},
        {"crop", QVersionNumber(0,6,2)},
        {"removeFile", QVersionNumber(0,6,2)},
        {"copyFile", QVersionNumber(0,6,2)},
        {"moveFile", QVersionNumber(0,6,2)},
        {"jumpToFirst", QVersionNumber(0,6,2)},
        {"jumpToLast", QVersionNumber(0,6,2)},
        {"openSettings", QVersionNumber(0,6,2)},
        {"closeFullScreenOrExit", QVersionNumber(0,6,2)},
        {"exit", QVersionNumber(0,6,2)},
        {"flipH", QVersionNumber(0,6,3)},
        {"flipV", QVersionNumber(0,6,3)},
        {"pauseVideo", QVersionNumber(0,6,85)},
        {"frameStep", QVersionNumber(0,6,85)},
        {"frameStepBack", QVersionNumber(0,6,85)},
        {"documentView", QVersionNumber(0,6,88)},
        {"toggleFolderView", QVersionNumber(0,6,88)},
        {"moveToTrash", QVersionNumber(0,6,89)},
        {"reloadImage", QVersionNumber(0,7,80)},
        {"copyFileClipboard", QVersionNumber(0,7,80)},
        {"copyPathClipboard", QVersionNumber(0,7,80)},
        {"renameFile", QVersionNumber(0,7,80)},
        {"contextMenu", QVersionNumber(0,7,81)},
        {"toggleTransparencyGrid", QVersionNumber(0,7,82)},
        {"sortByName", QVersionNumber(0,7,83)},
        {"sortByTime", QVersionNumber(0,7,83)},
        {"sortBySize", QVersionNumber(0,7,83)},
        {"toggleImageInfo", QVersionNumber(0,7,84)},
        {"toggleShuffle", QVersionNumber(0,8,3)},
        {"toggleScalingFilter", QVersionNumber(0,8,3)},
        {"toggleMute", QVersionNumber(0,8,7)},
        {"volumeUp", QVersionNumber(0,8,7)},
        {"volumeDown", QVersionNumber(0,8,7)},
        {"toggleSlideshow", QVersionNumber(0,8,81)},
        {"showInDirectory", QVersionNumber(0,8,82)},
        {"goUp", QVersionNumber(0,9,2)},
        {"discardEdits", QVersionNumber(0,9,2)},
        {"nextDirectory", QVersionNumber(0,9,2)},
        {"prevDirectory", QVersionNumber(0,9,2)},
        {"seekVideoForward", QVersionNumber(0,9,2)},
        {"seekVideoBackward", QVersionNumber(0,9,2)},
        {"lockZoom", QVersionNumber(0,9,2)},
        {"lockView", QVersionNumber(0,9,2)},
        {"print", QVersionNumber(1,0,0)},
        {"toggleFullscreenInfoBar", QVersionNumber(1,0,0)},
        {"pasteFile", QVersionNumber(1,0,3)},
        {"minimize", QVersionNumber(1,0,0)}
    };
    // 缓存 key 列表，避免每次调用都遍历
    mActionList = mActions.keys();
}

