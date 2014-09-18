#! /bin/sh
source ../../../calligra_xgettext.sh

$EXTRACTRC ../../ui/widgets/KarbonPatternOptionsWidget.ui filterEffectTool/FilterEffectEditWidget.ui >> rc.cpp
calligra_xgettext KarbonTools.pot rc.cpp *.cpp *.h CalligraphyTool/*.cpp \
    CalligraphyTool/*.h filterEffectTool/*cpp \
    ../../ui/widgets/KarbonPatternOptionsWidget.cpp \
    ../../ui/widgets/KarbonGradientEditWidget.cpp

rm -f rc.cpp
