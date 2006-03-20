/*****
*
* TOra - An Oracle Toolkit for DBA's and developers
* Copyright (C) 2003-2005 Quest Software, Inc
* Portions Copyright (C) 2005 Other Contributors
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
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*      As a special exception, you have permission to link this program
*      with the Oracle Client libraries and distribute executables, as long
*      as you follow the requirements of the GNU GPL in regard to all of the
*      software in the executable aside from Oracle client libraries.
*
*      Specifically you are not permitted to link this program with the
*      Qt/UNIX, Qt/Windows or Qt Non Commercial products of TrollTech.
*      And you are not permitted to distribute binaries compiled against
*      these libraries without written consent from Quest Software, Inc.
*      Observe that this does not disallow linking to the Qt Free Edition.
*
*      You may link this product with any GPL'd Qt library such as Qt/Free
*
* All trademarks belong to their respective owners.
*
*****/

#include "utils.h"

#include "toconf.h"
#include "toconnection.h"
#include "tonoblockquery.h"
#include "toresultitem.h"
#include "toresultresources.h"
#include "tosql.h"
#include "totool.h"

#include <qgrid.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>

#include "toresultitem.moc"

static toSQL SQLResource("toResultResources:Information",
                         "SELECT 'Total' \"-\",         'per Execution' \"-\",                                                   'per Row processed' \"-\",\n"
                         "       Sorts,                 DECODE(Executions,0,'N/A',ROUND(Sorts/Executions,3)) \" \",         DECODE(Rows_Processed,0,'N/A',ROUND(Sorts/Rows_Processed,3)) \" \",\n"
                         "       Parse_Calls \"Parse\", DECODE(Executions,0,'N/A',ROUND(Parse_Calls/Executions,3)) \" \",   DECODE(Rows_Processed,0,'N/A',ROUND(Parse_Calls/Rows_Processed,3)) \" \",\n"
                         "       Disk_Reads,            DECODE(Executions,0,'N/A',ROUND(Disk_Reads/Executions,3)) \" \",    DECODE(Rows_Processed,0,'N/A',ROUND(Disk_Reads/Rows_Processed,3)) \" \",\n"
                         "       Buffer_Gets,           DECODE(Executions,0,'N/A',ROUND(Buffer_Gets/Executions,3)) \" \",   DECODE(Rows_Processed,0,'N/A',ROUND(Buffer_Gets/Rows_Processed,3)) \" \",\n"
                         "       Rows_Processed,        DECODE(Executions,0,'N/A',ROUND(Rows_Processed/Executions,3)) \" \",' ' \"-\",\n"
                         "       Executions,            ' ' \"-\",                                                          ' ' \"-\",\n"
                         "       ' ' \"-\",             ' ' \"-\",                                                          ' ' \"-\",\n"
                         "       Loads,                 First_Load_Time,                                                    Parsing_User_Id,\n"
                         "       Parsing_Schema_Id,     Users_Executing,                                                    Users_Opening,\n"
                         "       Open_Versions,         Sharable_Mem,                                                       Kept_Versions,\n"
                         "       Persistent_Mem,        Optimizer_Mode,                                                     Loaded_Versions,\n"
                         "       Runtime_Mem,           Serializable_Aborts,                                                Invalidations\n"
                         " FROM v$sqlarea WHERE Address||':'||Hash_Value = :f1<char[100]>",
                         "Display information about an SQL statement");

toResultResources::toResultResources(QWidget *parent, const char *name)
        : toResultItem(3, true, parent, name)
{
    setSQL(SQLResource);
}

void toResultItem::setup(int num, bool readable)
{
    enableClipper(true);
    ReadableColumns = readable;
    NumWidgets = 0;
    WidgetPos = 0;
    viewport()->setBackgroundMode(PaletteBackground);
    Result = new QGrid(2 * num, viewport());
    Result->setFixedWidth(width() - 30);
    addChild(Result);
    Result->setSpacing(3);
    ShowTitle = true;
    Right = true;
    DataFont.setBold(true);
    Query = NULL;
    connect(&Poll, SIGNAL(timeout()), this, SLOT(poll()));
}

toResultItem::toResultItem(int num, bool readable, QWidget *parent, const char *name)
        : QScrollView(parent, name), DataFont(QFont::defaultFont())
{
    setup(num, readable);
}

toResultItem::toResultItem(int num, QWidget *parent, const char *name)
        : QScrollView(parent, name), DataFont(QFont::defaultFont())
{
    setup(num, false);
}

toResultItem::~toResultItem()
{
    delete Query;
}

void toResultItem::start(void)
{
    WidgetPos = 0;
}

// Must be alloced in multiples of 2
#define ALLOC_SIZE 1000

QWidget *toResultItem::createTitle(QWidget *parent)
{
    QLabel *widget = new QLabel(parent);
    widget->setAlignment(AlignRight | AlignVCenter | ExpandTabs | WordBreak);
    return widget;
}

QWidget *toResultItem::createValue(QWidget *parent)
{
    QLabel *widget = new QLabel(parent);
    return widget;
}

void toResultItem::setTitle(QWidget *widget, const QString &title, const QString &)
{
    QLabel *label = static_cast<QLabel *>(widget);
    if (label)
        label->setText(title);
}

void toResultItem::setValue(QWidget *widget, const QString &title, const QString &value)
{
    QLabel *label = static_cast<QLabel *>(widget);
    if (label)
    {
        if (title != "-")
        {
            label->setFrameStyle(StyledPanel | Sunken);
            label->setFont(DataFont);
        }
        else
        {
            label->setFrameStyle(NoFrame);
            label->setFont(qApp->font());
        }
        label->setText(value);
        if (Right)
            label->setAlignment(AlignRight | AlignVCenter | ExpandTabs | WordBreak);
        else
            label->setAlignment(AlignLeft | AlignVCenter | ExpandTabs | WordBreak);
    }
}

void toResultItem::addItem(const QString &title, const QString &value)
{
    if (WidgetPos >= NumWidgets)
    {
        NumWidgets += ALLOC_SIZE;
        Widgets.resize(NumWidgets, 0);
    }
    QString t;
    if (title != "-")
        t = toTranslateMayby(sqlName(), title);
    QWidget *widget;
    if (!Widgets[WidgetPos])
    {
        widget = createTitle(Result);
        Widgets[WidgetPos] = widget;
    }
    else
        widget = ((QLabel *)Widgets[WidgetPos]);

    setTitle(widget, t, value);
    if (ShowTitle)
        widget->show();
    else
        widget->hide();

    WidgetPos++;
    if (!Widgets[WidgetPos])
    {
        widget = createValue(Result);
        Widgets[WidgetPos] = widget;
    }
    else
        widget = Widgets[WidgetPos];
    setValue(widget, title, value);

    widget->show();
    WidgetPos++;
}

void toResultItem::done(void)
{
    for (int i = WidgetPos;i < NumWidgets;i++)
        if (Widgets[i])
            Widgets[i]->hide();
    QLayout *l = layout();
    if (l)
        l->invalidate();
}

void toResultItem::query(const QString &sql, const toQList &param)
{
    if (!setSQLParams(sql, param))
        return ;

    start();
    if (!handled() || Query)
    {
        if (!Query)
            done();
        return ;
    }

    try
    {
        if (Query)
        {
            delete Query;
            Query = NULL;
        }
        Query = new toNoBlockQuery(connection(), toQuery::Background,
                                   sql, param);
        Poll.start(100);

    }
    catch (const QString &str)
    {
        done();
        toStatusMessage(str);
    }
}

void toResultItem::poll(void)
{
    try
    {
        if (!toCheckModal(this))
            return ;
        if (Query && Query->poll())
        {
            toQDescList desc = Query->describe();

            if (!Query->eof())
            {
                for (toQDescList::iterator i = desc.begin();i != desc.end();i++)
                {
                    QString name = (*i).Name;
                    if (ReadableColumns)
                        toReadableColumn(name);

                    addItem(name, Query->readValue());
                }
            }
            done();
            delete Query;
            Query = NULL;
            Poll.stop();
        }
    }
    catch (const QString &str)
    {
        delete Query;
        Query = NULL;
        done();
        toStatusMessage(str);
        Poll.stop();
    }
}

void toResultItem::resizeEvent(QResizeEvent *e)
{
    QScrollView::resizeEvent(e);
    Result->setFixedWidth(width() - 30);
}