/********************************************************************************
** Form generated from reading UI file 'N3VMeshEditor.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_N3VMESHEDITOR_H
#define UI_N3VMESHEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "QDirect3D11Widget.h"

QT_BEGIN_NAMESPACE

class Ui_N3VMeshEditorClass
{
public:
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    QSplitter *mainSplitter;
    QWidget *leftPanelWidget;
    QVBoxLayout *verticalLayout_2;
    QTreeWidget *treeWidgetObjects;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *openShapeButton;
    QPushButton *openCollisionButton;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *addCollisionButton;
    QPushButton *removeCollisionButton;
    QPushButton *saveAllButton;
    QSpacerItem *verticalSpacer;
    QDirect3D11Widget *d3d11Widget;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *N3VMeshEditorClass)
    {
        if (N3VMeshEditorClass->objectName().isEmpty())
            N3VMeshEditorClass->setObjectName("N3VMeshEditorClass");
        N3VMeshEditorClass->resize(900, 600);
        N3VMeshEditorClass->setMinimumSize(QSize(600, 0));
        centralWidget = new QWidget(N3VMeshEditorClass);
        centralWidget->setObjectName("centralWidget");
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        mainSplitter = new QSplitter(centralWidget);
        mainSplitter->setObjectName("mainSplitter");
        mainSplitter->setMinimumSize(QSize(450, 0));
        mainSplitter->setOrientation(Qt::Orientation::Horizontal);
        mainSplitter->setChildrenCollapsible(false);
        leftPanelWidget = new QWidget(mainSplitter);
        leftPanelWidget->setObjectName("leftPanelWidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(leftPanelWidget->sizePolicy().hasHeightForWidth());
        leftPanelWidget->setSizePolicy(sizePolicy);
        leftPanelWidget->setMinimumSize(QSize(300, 0));
        verticalLayout_2 = new QVBoxLayout(leftPanelWidget);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        treeWidgetObjects = new QTreeWidget(leftPanelWidget);
        treeWidgetObjects->setObjectName("treeWidgetObjects");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(1);
        sizePolicy1.setHeightForWidth(treeWidgetObjects->sizePolicy().hasHeightForWidth());
        treeWidgetObjects->setSizePolicy(sizePolicy1);

        verticalLayout_2->addWidget(treeWidgetObjects);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        openShapeButton = new QPushButton(leftPanelWidget);
        openShapeButton->setObjectName("openShapeButton");

        horizontalLayout_2->addWidget(openShapeButton);

        openCollisionButton = new QPushButton(leftPanelWidget);
        openCollisionButton->setObjectName("openCollisionButton");

        horizontalLayout_2->addWidget(openCollisionButton);


        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        addCollisionButton = new QPushButton(leftPanelWidget);
        addCollisionButton->setObjectName("addCollisionButton");

        horizontalLayout_3->addWidget(addCollisionButton);

        removeCollisionButton = new QPushButton(leftPanelWidget);
        removeCollisionButton->setObjectName("removeCollisionButton");

        horizontalLayout_3->addWidget(removeCollisionButton);


        verticalLayout_2->addLayout(horizontalLayout_3);

        saveAllButton = new QPushButton(leftPanelWidget);
        saveAllButton->setObjectName("saveAllButton");

        verticalLayout_2->addWidget(saveAllButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        mainSplitter->addWidget(leftPanelWidget);
        d3d11Widget = new QDirect3D11Widget(mainSplitter);
        d3d11Widget->setObjectName("d3d11Widget");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(3);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(d3d11Widget->sizePolicy().hasHeightForWidth());
        d3d11Widget->setSizePolicy(sizePolicy2);
        d3d11Widget->setMinimumSize(QSize(300, 0));
        mainSplitter->addWidget(d3d11Widget);

        horizontalLayout->addWidget(mainSplitter);

        N3VMeshEditorClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(N3VMeshEditorClass);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 900, 33));
        N3VMeshEditorClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(N3VMeshEditorClass);
        mainToolBar->setObjectName("mainToolBar");
        N3VMeshEditorClass->addToolBar(Qt::ToolBarArea::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(N3VMeshEditorClass);
        statusBar->setObjectName("statusBar");
        N3VMeshEditorClass->setStatusBar(statusBar);

        retranslateUi(N3VMeshEditorClass);

        QMetaObject::connectSlotsByName(N3VMeshEditorClass);
    } // setupUi

    void retranslateUi(QMainWindow *N3VMeshEditorClass)
    {
        N3VMeshEditorClass->setWindowTitle(QCoreApplication::translate("N3VMeshEditorClass", "N3VMesh Editor", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidgetObjects->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("N3VMeshEditorClass", "Object Name", nullptr));
        openShapeButton->setText(QCoreApplication::translate("N3VMeshEditorClass", "Open Shape...", nullptr));
        openCollisionButton->setText(QCoreApplication::translate("N3VMeshEditorClass", "Open Collision Mesh...", nullptr));
        addCollisionButton->setText(QCoreApplication::translate("N3VMeshEditorClass", "Add Collision", nullptr));
        removeCollisionButton->setText(QCoreApplication::translate("N3VMeshEditorClass", "Remove Collision", nullptr));
        saveAllButton->setText(QCoreApplication::translate("N3VMeshEditorClass", "Save All", nullptr));
        mainToolBar->setWindowTitle(QCoreApplication::translate("N3VMeshEditorClass", "mainToolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class N3VMeshEditorClass: public Ui_N3VMeshEditorClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_N3VMESHEDITOR_H
