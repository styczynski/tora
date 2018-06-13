
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

#pragma once

#include "core/toqvalue.h"
#include "core/tosql.h"
#include "core/toresult.h"
#include "core/utils.h"
#include "core/toconnection.h"
#include "widgets/toresultmodel.h"
#include "core/toeditwidget.h"

#include <QtCore/QAbstractTableModel>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLabel>
#include <QtCore/QList>
#include <QMenu>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QPushButton>
#include <QListView>
#include <QString>
#include <QGroupBox>
#include <QMenuBar>
#include <QVBoxLayout>
#include <chrono>

class MultiResult
{
    public:
        
        MultiResult();
        QString getName() const;
        void setName(QString name);
        bool isDone() const;
        
        void setStatusDone();
        void setStatusExecuting();
        
        std::chrono::high_resolution_clock::time_point getCreationTime() const;
        void resetCreationTime();
        
        inline bool operator!=(const MultiResult& mr) const {
            return (name_ == mr.name_ && status_ == mr.status_);
        }
        
    private:
    
        QString name_;
        bool status_;
        std::chrono::high_resolution_clock::time_point creation_time_;
};

class MultiResultListModel : public QAbstractListModel
{
    
    Q_OBJECT

    public:

        explicit MultiResultListModel(const std::vector<MultiResult>& results, QObject* parent = 0);
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role) const;
        
    private:
        
        std::vector<MultiResult> results_;

};

class toMultiResultList : public QListView
{
    Q_OBJECT;
    
    private:
        static std::map<int, MultiResult> resultSet_;
    
    public:
        toMultiResultList(QWidget *parent = nullptr);
        void updateStatus(int id, MultiResult resul);
        void clearStatus();
        
};

class toMultiResultTableView : public QGroupBox
{
    Q_OBJECT;
    
    private:
        toMultiResultList* list;
    
    private slots:
        void slotClearAction(void);
    
    public:
        toMultiResultTableView(QWidget *parent = nullptr);
        void updateStatus(int id, MultiResult resul);
        void clearStatus();
};