
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

#include "tools/tomulticonnectdialog.h"

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

toMultiConnectionChooserDialog::toMultiConnectionChooserDialog(QWidget *parent) : QDialog(parent) {
    list = new toMultiResultTableView(this, true);
    
    confirmButton = new QPushButton(tr("&Ok"));
    confirmButton->setDefault(true);

    connect(confirmButton, SIGNAL(clicked()), this, SLOT(confirmClicked()));

    mainLayout = new QHBoxLayout;
    
    mainLayout->addWidget(confirmButton);
    mainLayout->addWidget(list);
     
    setLayout(mainLayout);
    setWindowTitle(tr("Choose current connections"));
    
    MultiResult mr;
    list->updateStatus(-1, mr);
}

void toMultiConnectionChooserDialog::confirmClicked(void) {
    hide();
}

std::vector<int> toMultiConnectionChooserDialog::getSelectedConnections() {
    std::vector<int> selectedConnections;
    
    for(std::pair<int, MultiResult> result : list->getConnections()) {
        if(result.second.isSelected()) {
            selectedConnections.push_back(result.first);
        }
    }
    
    return selectedConnections;
}