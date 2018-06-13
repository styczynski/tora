
/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * TOra - An Oracle Toolkit for DBA's and developers
 *
 * Shared/mixed copyright is held throughout files in this product
 *
 * Portions Copyright (C) 2000-2001 Underscore AB
 * Portions Copyright (C) 2003-2005 Quest Software, Inc.
 * Portions Copyright (C) 2004-2013 Numerous Other Contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;  only version 2 of
 * the License is valid for this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program as the file COPYING.txt; if not, please see
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *      As a special exception, you have permission to link this program
 *      with the Oracle Client libraries and distribute executables, as long
 *      as you follow the requirements of the GNU GPL in regard to all of the
 *      software in the executable aside from Oracle client libraries.
 *
 * All trademarks belong to their respective owners.
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "tools/tomultiresulttableview.h"

#include "widgets/toresultmodel.h"
#include "core/toeventquery.h"
#include "core/utils.h"
#include "core/toconfiguration.h"
#include "core/toconnection.h"
#include "editor/tomodeleditor.h"
#include "core/tomainwindow.h"
#include "widgets/toresultlistformat.h"
#include "core/tolistviewformatter.h"
#include "core/tolistviewformatterfactory.h"
#include "core/tolistviewformatteridentifier.h"
#include "widgets/toworkingwidget.h"
#include "core/toglobalconfiguration.h"
#include "core/todatabaseconfig.h"
#include "core/tocontextmenu.h"

#include <QtCore/QSize>
#include <QtCore/QTimer>
#include <QtCore/QtDebug>
#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QProgressDialog>
#include <QListView>

#include "icons/execute.xpm"
#include "icons/clock.xpm"

MultiResult::MultiResult() {
    name_ = QString("");
    status_ = false;
    creation_time_ = std::chrono::high_resolution_clock::now();
}

std::chrono::high_resolution_clock MultiResult::getCreationTime() {
    return creation_time_;
}

void MultiResult::resetCreationTime() {
    creation_time_ = std::chrono::high_resolution_clock::now();
}

QString MultiResult::getName() const {
    return name_;
}

void MultiResult::setName(QString name) {
    name_ = name;
}

bool MultiResult::isDone() const {
    return status_;
}

void MultiResult::setStatusDone() {
    status_ = true;
}

void MultiResult::setStatusExecuting() {
    status_ = false;
}

std::map<int, MultiResult> toMultiResultList::resultSet_;

MultiResultListModel::MultiResultListModel(const std::vector<MultiResult>& results, QObject *parent) : QAbstractListModel(parent) {
    results_ = results;
}

int MultiResultListModel::rowCount(const QModelIndex& parent) const {
    return results_.size();
}

QVariant MultiResultListModel::data(const QModelIndex& index, int role) const {
    // Check that the index is valid and within the correct range first:
    if(!index.isValid()) return QVariant();
    //if (index.row() >= decks_.size()) return QVariant();

    if(role == Qt::DisplayRole) {
        // Only returns something for the roles you support (DisplayRole is a minimum)
        // Here we assume that the "MultiResult" class has a "lastName" method but of course any string can be returned
        QString label = results_.at(index.row()).getName();
        
        if(results_.at(index.row()).isDone()) {
           auto now = std::chrono::high_resolution_clock::now();
           label +=
             std::string(" - Ready (started ")
             + std::to_string(
                std::chrono::duration_cast<std::chrono::nanoseconds>(now - results_.at(index.row()).getCreationTime()).count()
                / 1000
             )
             + " ms ago)";
        } else {
           label += " - Running ";
        }
        
        return QVariant(label);
    } else if(role == Qt::DecorationRole) {
        if(results_.at(index.row()).isDone()) {
            return QPixmap(const_cast<const char**>(execute_xpm));
        }
        return QPixmap(const_cast<const char**>(clock_xpm));
    } else {
        return QVariant();
    }
}

toMultiResultList::toMultiResultList(QWidget *parent) : QListView(parent) {
    
    std::vector<MultiResult> results;
    MultiResultListModel* model = new MultiResultListModel(results);
    setModel(model);
}

void toMultiResultList::updateStatus(int id, MultiResult result) {
    
    if(resultset_[id] != result) {
        result.resetCreationTime();
        resultSet_[id] = result;
    }
    
    std::vector<MultiResult> results;
    for(std::pair<int, MultiResult> result : resultSet_) {
       MultiResult mr = result.second;
       results.push_back(mr);
    }
    
    MultiResultListModel* model = new MultiResultListModel(results);
    setModel(model);
}

void toMultiResultList::clearStatus() {
    resultSet_.clear();
    std::vector<MultiResult> results;
    MultiResultListModel* model = new MultiResultListModel(results);
    setModel(model);
}


toMultiResultTableView::toMultiResultTableView(QWidget *parent) : QVBoxLayout(parent) {
    QMenuBar* menuBar = new QMenuBar;

    quickOptionsMenu = new QMenu(tr("&Quick options"), this);
    
    clearAction = quickOptionsMenu->addAction(tr("&Clear"));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(slotClearAction()));
    
    menuBar->addMenu(quickOptionsMenu);
    setMenuBar(menuBar);
    
    list = new toMultiResultList(this);
    addWidget(list);
}

void toMultiResultTableView::slotClearAction(void) {
    clearStatus();
}

void toMultiResultTableView::updateStatus(int id, MultiResult result) {
    list->updateStatus(id, result);
}

void toMultiResultTableView::clearStatus() {
    list->clearStatus();
}