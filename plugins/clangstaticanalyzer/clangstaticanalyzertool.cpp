/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzertool.h"

#include "clangstaticanalyzerdiagnosticmodel.h"
#include "clangstaticanalyzerdiagnosticview.h"
#include "clangstaticanalyzerruncontrol.h"

#include <analyzerbase/analyzermanager.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <utils/fancymainwindow.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QToolButton>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerTool::ClangStaticAnalyzerTool(QObject *parent)
    : IAnalyzerTool(parent)
    , m_diagnosticModel(0)
    , m_diagnosticView(0)
    , m_goBack(0)
    , m_goNext(0)
{
    setObjectName(QLatin1String("ClangStaticAnalyzerTool"));
    setRunMode(ProjectExplorer::ClangStaticAnalyzerMode);
    setToolMode(AnyMode);
}

QWidget *ClangStaticAnalyzerTool::createWidgets()
{
    QTC_ASSERT(!m_diagnosticView, return 0);
    QTC_ASSERT(!m_diagnosticModel, return 0);
    QTC_ASSERT(!m_goBack, return 0);
    QTC_ASSERT(!m_goNext, return 0);

    //
    // Diagnostic View
    //
    m_diagnosticView = new DetailedErrorView;
    m_diagnosticView->setItemDelegate(new ClangStaticAnalyzerDiagnosticDelegate(m_diagnosticView));
    m_diagnosticView->setObjectName(QLatin1String("ClangStaticAnalyzerIssuesView"));
    m_diagnosticView->setFrameStyle(QFrame::NoFrame);
    m_diagnosticView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_diagnosticModel = new ClangStaticAnalyzerDiagnosticModel(m_diagnosticView);
    // TODO: Make use of the proxy model
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(m_diagnosticView);
    proxyModel->setSourceModel(m_diagnosticModel);
    m_diagnosticView->setModel(proxyModel);
    m_diagnosticView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_diagnosticView->setAutoScroll(false);
    m_diagnosticView->setObjectName(QLatin1String("ClangStaticAnalyzerIssuesView"));
    m_diagnosticView->setWindowTitle(tr("Clang Static Analyzer Issues"));

    QDockWidget *issuesDock = AnalyzerManager::createDockWidget(this, m_diagnosticView);
    issuesDock->show();
    Utils::FancyMainWindow *mw = AnalyzerManager::mainWindow();
    mw->splitDockWidget(mw->toolBarDockWidget(), issuesDock, Qt::Vertical);

    //
    // Toolbar widget
    //
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    QAction *action = 0;
    QToolButton *button = 0;

    // Go to previous diagnostic
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PREV)));
    action->setToolTip(tr("Go to previous bug."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goBack);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_goBack = action;

    // Go to next diagnostic
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_NEXT)));
    action->setToolTip(tr("Go to next bug."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goNext);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_goNext = action;

    layout->addStretch();

    QWidget *toolbarWidget = new QWidget;
    toolbarWidget->setObjectName(QLatin1String("ClangStaticAnalyzerToolBarWidget"));
    toolbarWidget->setLayout(layout);
    return toolbarWidget;
}

AnalyzerRunControl *ClangStaticAnalyzerTool::createRunControl(
        const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration)
{
    ClangStaticAnalyzerRunControl *engine = new ClangStaticAnalyzerRunControl(sp, runConfiguration);
    connect(engine, &ClangStaticAnalyzerRunControl::starting,
            this, &ClangStaticAnalyzerTool::onEngineIsStarting);
    connect(engine, &ClangStaticAnalyzerRunControl::newDiagnosticsAvailable,
            this, &ClangStaticAnalyzerTool::onNewDiagnosticsAvailable);
    connect(engine, &ClangStaticAnalyzerRunControl::finished,
            this, &ClangStaticAnalyzerTool::onEngineFinished);
    return engine;
}

void ClangStaticAnalyzerTool::startTool(StartMode mode)
{
    QTC_ASSERT(mode == Analyzer::StartLocal, return);

    AnalyzerManager::showMode();
    if (Project *pro = SessionManager::startupProject())
        ProjectExplorerPlugin::instance()->runProject(pro, runMode());
}

void ClangStaticAnalyzerTool::onEngineIsStarting()
{
    QTC_ASSERT(m_diagnosticModel, return);
    m_diagnosticModel->clear();
    setBusyCursor(true);
}

void ClangStaticAnalyzerTool::onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics)
{
    QTC_ASSERT(m_diagnosticModel, return);
    m_diagnosticModel->addDiagnostics(diagnostics);
}

void ClangStaticAnalyzerTool::onEngineFinished()
{
    QTC_ASSERT(m_goBack, return);
    QTC_ASSERT(m_goNext, return);
    QTC_ASSERT(m_diagnosticModel, return);
    const int issuesFound = m_diagnosticModel->rowCount();
    m_goBack->setEnabled(issuesFound > 1);
    m_goNext->setEnabled(issuesFound > 1);
    setBusyCursor(false);

    AnalyzerManager::showStatusMessage(issuesFound > 0
      ? AnalyzerManager::tr("Clang Static Analyzer finished, %n issues were found.", 0, issuesFound)
      : AnalyzerManager::tr("Clang Static Analyzer finished, no issues were found."));
}

void ClangStaticAnalyzerTool::setBusyCursor(bool busy)
{
    QTC_ASSERT(m_diagnosticView, return);
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_diagnosticView->setCursor(cursor);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer