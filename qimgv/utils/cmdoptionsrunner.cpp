#include "cmdoptionsrunner.h"
#include <QStringBuilder>

void CmdOptionsRunner::showBuildOptions() {
    QStringList features;
#ifdef USE_MPV
    features << "USE_MPV";
#endif
#ifdef USE_KDE_BLUR
    features << "USE_KDE_BLUR";
#endif
#ifdef USE_OPENCV
    features << "USE_OPENCV";
#endif
    
    qDebug() << "\nEnabled build options:";
    if(!features.count()) {
        qDebug() << "   --";
    } else {
        for(const auto& feature : features) {
            qDebug() << "   " << feature;
        }
    }
    QCoreApplication::quit();
}
