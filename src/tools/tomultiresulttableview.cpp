
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
#include "icons/refresh.xpm"

MultiResult::MultiResult() {
    name_ = QString("");
    status_ = false;
    selected_ = false;
    creation_time_ = std::chrono::high_resolution_clock::now();
    execution_time_ = -1;
}

std::chrono::high_resolution_clock::time_point MultiResult::getCreationTime() const {
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

void MultiResult::setSelected(bool state) {
    selected_ = state;
}

bool MultiResult::isSelected() const {
    return selected_;
}

MultiResultListModel::MultiResultListModel(const std::vector<MultiResult>& results, QObject *parent) : QAbstractListModel(parent) {
    results_ = results;
}

int MultiResultListModel::rowCount(const QModelIndex& parent) const {
    return results_.size();
}

void MultiResultListModel::setCheckMode(bool mode) {
    checkMode_ = mode;
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
           label += QString(" - Ready ");
           
           if(results_.at(index.row()).getExecutionTime() > 0) {
               label += QString("(took ") + QString(
                     std::to_string(
                        results_.at(index.row()).getExecutionTime()
                     ).c_str()
                 ) + " ms)";
           }
        } else {
           label += " - Running ";
        }
        
        return QVariant(label);
    } else if(role == Qt::DecorationRole) {
        if(results_.at(index.row()).isDone()) {
            return QPixmap(const_cast<const char**>(execute_xpm));
        }
        return QPixmap(const_cast<const char**>(clock_xpm));
    } else if(role == Qt::CheckStateRole) {
        if(!checkMode_) return QVariant();
        if(results_.at(index.row()).isSelected()) {
            return QVariant(Qt::Checked);
        }
        return QVariant(Qt::Unchecked);
    } else if(role == Qt::UserRole) {
        return QVariant(
            QString(
                std::to_string(
                    index.row()
                ).c_str()
            )
        );
    } else {
        return QVariant();
    }
}

std::vector<toMultiResultTableView*> toMultiResultTableView::views_;

void toMultiResultTableView::refreshModel() {
    
   const int len = views_.size();
   for(int i=0; i<len; ++i) {
       if(views_[i] != NULL) {
           std::vector<MultiResult> results;
           for(std::pair<int, MultiResult> result : views_[i]->resultSet_) {
              MultiResult mr = result.second;
              results.push_back(mr);
           }
           
           views_[i]->model_ = new MultiResultListModel(results);
           views_[i]->model_->setCheckMode(views_[i]->checkMode_);
           views_[i]->setModel(views_[i]->model_);
       }
   }
}

toMultiResultTableView::toMultiResultTableView(QWidget *parent, bool checkMode) : QListView(parent) {
    
    views_.push_back(this);

    checkMode_ = checkMode;
    model_ = NULL;
    
    refreshModel();
    
    connect(this, SIGNAL(clicked(const QModelIndex)), this, SLOT(slotItemClicked(QModelIndex)));
}

toMultiResultTableView::~toMultiResultTableView() {
    views_.erase(std::remove(views_.begin(), views_.end(), this), views_.end());
}

void toMultiResultTableView::slotItemClicked(QModelIndex item) {
    QString idStr = item.data(Qt::UserRole).value<QString>();
    
    std::cerr << "slotItemClicked:  {";
    std::cerr << idStr.toUtf8().constData();
    std::cerr << "}\n";
    
    int id = std::stoi(std::string(idStr.toUtf8().constData()));
    
    int itemIndex = -1;
    for(std::pair<int, MultiResult> result : resultSet_) {
       std::cerr << "HERE: {";
       std::cerr << result.second.getName().toUtf8().constData() << "}\n";
       
       itemIndex = result.first;
       if(result.first == id) {
           break;
       }
    }
    
   std::cerr << "END AND RENDER ";
   std::cerr << " i = " << itemIndex << "\n";
             
   if(itemIndex >= 0) {
        resultSet_[itemIndex].setSelected(!resultSet_[itemIndex].isSelected());
   }
             
   refreshModel();
   
}

std::map<int, MultiResult> toMultiResultTableView::getConnections() const {
    return resultSet_;
}

void toMultiResultTableView::updateStatus(int id, MultiResult result) {
    
    if(id != -1) {
        if(!resultSet_[id].isDone() && result.isDone()) {
            result.setExecutionTime(
                std::chrono::duration_cast<std::chrono::nanoseconds>(result.getCreationTime() - resultSet_[id].getCreationTime()).count()
                / 1000000
            );
        }
        
        bool selectedStateBefore = resultSet_[id].isSelected();
        result.setSelected(selectedStateBefore);
        
        resultSet_[id] = result;
    }
    
    refreshModel();
    
}

void toMultiResultTableView::clearStatus() {
    resultSet_.clear();
    refreshModel();
}

/*
toMultiResultTableView::toMultiResultTableView(QWidget *parent) : QGroupBox(parent) {
    QHBoxLayout* layout = new QHBoxLayout;
    *QMenuBar* menuBar = new QMenuBar;

    QMenu* quickOptionsMenu = new QMenu(tr("&Quick options"), this);
    
    QAction* clearAction = new QAction(QPixmap(const_cast<const char**>(refresh_xpm)),
                                 tr("Clear list"),
                                 this);
    
    quickOptionsMenu->addAction(clearAction);
    connect(clearAction, SIGNAL(triggered()), this, SLOT(slotClearAction()));
    
    menuBar->addMenu(quickOptionsMenu);
    layout->setMenuBar(menuBar);*
    
    list = new toMultiResultTableView(this);
    layout->addWidget(list);
    
    setLayout(layout);
}

void toMultiResultTableView::slotClearAction(void) {
    clearStatus();
}

void toMultiResultTableView::updateStatus(int id, MultiResult result) {
    list->updateStatus(id, result);
}

void toMultiResultTableView::clearStatus() {
    list->clearStatus();
}*/