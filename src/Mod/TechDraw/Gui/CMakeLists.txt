if(MSVC)
    add_definitions(-DHAVE_ATANH -DHAVE_ASINH -DHAVE_ACOSH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(TechDrawGui_LIBS
    TechDraw
    FreeCADGui
)


set(TechDrawGui_MOC_HDRS
    MDIViewPage.h
    QGVPage.h
    QGITemplate.h
    QGISVGTemplate.h
    QGIDrawingTemplate.h
    QGIViewDimension.h
    TaskProjGroup.h
    DlgPrefsTechDrawImp.h
    DlgPrefsTechDraw2Imp.h
    TaskLinkDim.h
    DlgTemplateField.h
    TaskSectionView.h
)

fc_wrap_cpp(TechDrawGui_MOC_SRCS ${TechDrawGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${TechDrawGui_MOC_SRCS})

qt4_add_resources(TechDrawGui_SRCS Resources/TechDraw.qrc)

set(TechDrawGui_UIC_SRCS
    DlgPrefsTechDraw.ui
    DlgPrefsTechDraw2.ui
    TaskProjGroup.ui
    TaskLinkDim.ui
    DlgTemplateField.ui
    TaskSectionView.ui
)

qt4_wrap_ui(TechDrawGui_UIC_HDRS ${TechDrawGui_UIC_SRCS})

SET(TechDrawGui_SRCS
    ${TechDrawGui_SRCS}
    AppTechDrawGui.cpp
    AppTechDrawGuiPy.cpp
    Command.cpp
    CommandCreateDims.cpp
    CommandDecorate.cpp
    Resources/TechDraw.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    TaskProjGroup.ui
    TaskProjGroup.cpp
    TaskProjGroup.h
    DlgPrefsTechDraw.ui
    DlgPrefsTechDrawImp.cpp
    DlgPrefsTechDrawImp.h
    DlgPrefsTechDraw2.ui
    DlgPrefsTechDraw2Imp.cpp
    DlgPrefsTechDraw2Imp.h
    TaskLinkDim.ui
    TaskLinkDim.cpp
    TaskLinkDim.h
    DlgTemplateField.ui
    DlgTemplateField.cpp
    DlgTemplateField.h
    TaskSectionView.ui
    TaskSectionView.cpp
    TaskSectionView.h
    DrawGuiUtil.cpp
    DrawGuiUtil.h
)
SET(TechDrawGuiView_SRCS
    MDIViewPage.cpp
    MDIViewPage.h
    QGVPage.cpp
    QGVPage.h
    QGCustomText.cpp
    QGCustomText.h
    QGICaption.cpp
    QGICaption.h
    QGCustomRect.cpp
    QGCustomRect.h
    QGCustomSvg.cpp
    QGCustomSvg.h
    QGCustomClip.cpp
    QGCustomClip.h
    QGCustomLabel.cpp
    QGCustomLabel.h
    QGCustomBorder.cpp
    QGCustomBorder.h
    QGCustomImage.cpp
    QGCustomImage.h
    QGIView.cpp
    QGIView.h
    QGIArrow.cpp
    QGIArrow.h
    QGIEdge.cpp
    QGIEdge.h
    QGIFace.cpp
    QGIFace.h
    QGISVGTemplate.cpp
    QGISVGTemplate.h
    QGIVertex.cpp
    QGIVertex.h
    QGIMatting.cpp
    QGIMatting.h
    QGIDrawingTemplate.cpp
    QGIDrawingTemplate.h
    QGITemplate.cpp
    QGITemplate.h
    QGIViewCollection.cpp
    QGIViewCollection.h
    QGIViewDimension.cpp
    QGIViewDimension.h
    QGIProjGroup.cpp
    QGIProjGroup.h
    QGIViewPart.cpp
    QGIViewPart.h
    QGIViewSection.cpp
    QGIViewSection.h
    QGIViewAnnotation.cpp
    QGIViewAnnotation.h
    QGIViewSymbol.cpp
    QGIViewSymbol.h
    QGIViewSpreadsheet.cpp
    QGIViewSpreadsheet.h
    QGIViewImage.cpp
    QGIViewImage.h
    QGIViewClip.cpp
    QGIViewClip.h
    QGIPrimPath.cpp
    QGIPrimPath.h
    QGICMark.cpp
    QGICMark.h
    QGIDimLines.cpp
    QGIDimLines.h
    QGISectionLine.cpp
    QGISectionLine.h
    QGIDecoration.cpp
    QGIDecoration.h
    QGICenterLine.cpp
    QGICenterLine.h
    TemplateTextField.cpp
    TemplateTextField.h
    ZVALUE.h
)
SET(TechDrawGuiViewProvider_SRCS
    ViewProviderPage.cpp
    ViewProviderPage.h
    ViewProviderDrawingView.cpp
    ViewProviderDrawingView.h
    ViewProviderProjGroupItem.cpp
    ViewProviderProjGroupItem.h
    ViewProviderTemplate.cpp
    ViewProviderTemplate.h
    ViewProviderDimension.cpp
    ViewProviderDimension.h
    ViewProviderViewPart.cpp
    ViewProviderViewPart.h
    ViewProviderProjGroup.cpp
    ViewProviderProjGroup.h
    ViewProviderViewSection.cpp
    ViewProviderViewSection.h
    ViewProviderAnnotation.cpp
    ViewProviderAnnotation.h
    ViewProviderSymbol.cpp
    ViewProviderSymbol.h
    ViewProviderSpreadsheet.cpp
    ViewProviderSpreadsheet.h
    ViewProviderViewClip.cpp
    ViewProviderViewClip.h
    ViewProviderHatch.cpp
    ViewProviderHatch.h
    ViewProviderImage.cpp
    ViewProviderImage.h
)

SOURCE_GROUP("Mod" FILES ${TechDrawGui_SRCS})
SOURCE_GROUP("SVG-View" FILES ${TechDrawGuiView_SRCS})
SOURCE_GROUP("ViewProvider" FILES ${TechDrawGuiViewProvider_SRCS})

SET(TechDrawGuiTaskDlgs_SRCS
    TaskProjGroup.ui
    TaskLinkDim.ui
    TaskSectionView.ui
)
SOURCE_GROUP("TaskDialogs" FILES ${TechDrawGuiTaskDlgs_SRCS})

if(MSVC)
    #add_definitions(-D_PreComp_)
    #GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${TechDrawGui_SRCS} ${TechDrawGuiView_SRCS} ${TechDrawGuiViewProvider_SRCS})
    #ADD_MSVC_PRECOMPILED_HEADER(TechDrawGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(MSVC)

add_library(TechDrawGui SHARED ${TechDrawGui_SRCS} ${TechDrawGuiView_SRCS} ${TechDrawGuiViewProvider_SRCS})
target_link_libraries(TechDrawGui ${TechDrawGui_LIBS})

fc_target_copy_resource(TechDrawGui
    ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw
    ${CMAKE_BINARY_DIR}/Mod/TechDraw
    InitGui.py)

SET(TechDrawGuiIcon_SVG
    Resources/icons/preferences-techdraw.svg
)

fc_copy_sources(TechDrawGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw" ${TechDrawGuiIcon_SVG})

INSTALL(FILES ${TechDrawGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/Resources/icons")

SET_BIN_DIR(TechDrawGui TechDrawGui /Mod/TechDraw)
SET_PYTHON_PREFIX_SUFFIX(TechDrawGui)

INSTALL(TARGETS TechDrawGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
