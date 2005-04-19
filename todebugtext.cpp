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

#include "toconnection.h"
#include "todebug.h"
#include "todebugtext.h"
#include "tosql.h"

#include <qapplication.h>
#include <qpainter.h>
#include <qpixmap.h>

#include "icons/breakpoint.xpm"
#include "icons/disbreakpoint.xpm"

#define TO_BREAK_COL 5

int toDebugText::ID = 0;

toBreakpointItem::toBreakpointItem(QListView *parent, QListViewItem *after,
                                   const QString &schema, const QString &type,
                                   const QString &object, int line)
        : QListViewItem(parent, after)
{
    if (schema.isNull())
        setText(2, QString::null);
    else
        setText(2, schema);
    if (object.isNull())
        setText(0, QString::null);
    else
        setText(0, object);
    if (type.isNull())
        setText(3, QString::null);
    else
        setText(3, type);
    setText(1, QString::number(line + 1));
    if (type == QString::fromLatin1("PACKAGE") ||
            type == QString::fromLatin1("PROCEDURE") ||
            type == QString::fromLatin1("FUNCTION") ||
            type == QString::fromLatin1("TYPE"))
        Namespace = TO_NAME_TOPLEVEL;
    else if (type == QString::fromLatin1("PACKAGE BODY") ||
             type == QString::fromLatin1("TYPE BODY"))
        Namespace = TO_NAME_BODY;
    else
        Namespace = TO_NAME_NONE;
    Line = line;
    setText(4, qApp->translate("toDebug", "DEFERED"));
}

static toSQL SQLBreakpoint("toDebug:SetBreakpoint",
                           "DECLARE\n"
                           "    proginf SYS.DBMS_DEBUG.PROGRAM_INFO;\n"
                           "    bnum BINARY_INTEGER;\n"
                           "    ret BINARY_INTEGER;\n"
                           "BEGIN\n"
                           "    proginf.Namespace:=:type<int,in>;\n"
                           "    proginf.Name:=:name<char[100],in>;\n"
                           "    proginf.Owner:=:schema<char[100],in>;\n"
                           "    proginf.DbLink:=NULL;\n"
                           "    proginf.LibUnitType:=SYS.DBMS_DEBUG.LibUnitType_Procedure;\n"
                           "    proginf.EntryPointName:=NULL;\n"
                           "    proginf.Line#:=:line<int,in>;\n"
                           "    ret:=SYS.DBMS_DEBUG.SET_BREAKPOINT(proginf,proginf.Line#,bnum,0,1);\n"
                           "    SELECT ret,bnum INTO :ret<int,out>,:bnum<int,out> FROM sys.DUAL;\n"
                           "END;",
                           "Set breakpoint, must have same bindings");


void toBreakpointItem::setBreakpoint(void)
{
    bool ok = false;
    try
    {
        clearBreakpoint();
        toConnection &conn = toCurrentConnection(listView());
        toQList args;
        toPush(args, toQValue(Namespace));
        toPush(args, toQValue(text(0)));
        toPush(args, toQValue(text(2)));
        toPush(args, toQValue(Line + 1));
        toQuery query(conn, SQLBreakpoint, args);
        int ret = query.readValue().toInt();
        if (ret == TO_SUCCESS)
        {
            setText(TO_BREAK_COL, query.readValue());
            setText(4, qApp->translate("toDebug", "ENABLED"));
            ok = true;
        }
        else if (ret == TO_ERROR_ILLEGAL_LINE)
        {
            toStatusMessage(qApp->translate("toDebug", "Can not enable breakpoint, not a valid line. Perhaps needs to recompile."));
        }
        else if (ret == TO_ERROR_BAD_HANDLE)
        {
            toStatusMessage(qApp->translate("toDebug", "Can not enable breakpoint, not a valid object. Perhaps needs to compile."));
        }
    }
    TOCATCH
    if (!ok)
        setText(4, qApp->translate("toDebug", "NOT SET"));
}

static toSQL SQLClearBreakpoint("toDebug:ClearBreakpoint",
                                "DECLARE\n"
                                "    bnum BINARY_INTEGER;\n"
                                "    ret BINARY_INTEGER;\n"
                                "BEGIN\n"
                                "    bnum:=:bnum<int,in>;\n"
                                "    ret:=SYS.DBMS_DEBUG.DELETE_BREAKPOINT(bnum);\n"
                                "    SELECT ret INTO :ret<int,out> FROM sys.DUAL;\n"
                                "END;",
                                "Clear breakpoint, must have same bindings");

void toBreakpointItem::clearBreakpoint()
{
    if (text(4) == qApp->translate("toDebug", "ENABLED") && !text(TO_BREAK_COL).isEmpty())
    {
        try
        {
            toConnection &conn = toCurrentConnection(listView());
            toQList args;
            toPush(args, toQValue(text(TO_BREAK_COL).toInt()));
            toQuery query(conn, SQLBreakpoint);
            int res = query.readValue().toInt();

            if (res != TO_SUCCESS)
            {
                toStatusMessage(qApp->translate("toDebug", "Failed to remove breakpoint (Reason %1)").arg(res));
            }
        }
        TOCATCH
        setText(TO_BREAK_COL, QString::null);
    }
    setText(4, qApp->translate("toDebug", "DISABLED"));
}

#define DEBUG_INDENT 10

static QPixmap *toBreakpointPixmap;
static QPixmap *toDisBreakpointPixmap;

static toSQL SQLReadSource("toDebug:ReadSource",
                           "SELECT Text FROM SYS.All_Source\n"
                           " WHERE OWNER = :f1<char[101]>\n"
                           "   AND NAME = :f2<char[101]>\n"
                           "   AND TYPE = :f3<char[101]>\n"
                           " ORDER BY Type,Line",
                           "Read sourcecode for object");
static toSQL SQLReadErrors("toDebug:ReadErrors",
                           "SELECT Line-1,Text FROM SYS.All_Errors\n"
                           " WHERE OWNER = :f1<char[101]>\n"
                           "   AND NAME = :f2<char[101]>\n"
                           "   AND TYPE = :f3<char[101]>\n"
                           " ORDER BY Type,Line",
                           "Get lines with errors in object (Observe first line 0)");

bool toDebugText::readErrors(toConnection &conn)
{
    try
    {
        toQuery errors(conn, SQLReadErrors, Schema, Object, Type);
        std::map<int, QString> Errors;

        while (!errors.eof())
        {
            int line = errors.readValue().toInt();
            Errors[line] += QString::fromLatin1(" ");
            Errors[line] += errors.readValue();
        }
        setErrors(Errors);
        return true;
    }
    TOCATCH
    return false;
}
bool toDebugText::readData(toConnection &conn, QListView *Stack)
{
    QListViewItem *item = NULL;
    if (Stack && Stack->firstChild())
        for (item = Stack->firstChild();item->firstChild();item = item->firstChild())
            ;
    try
    {
        toQuery lines(conn, SQLReadSource, Schema, Object, Type);

        QString str;
        while (!lines.eof())
            str += lines.readValue();
        setText(str);
        setEdited(false);
        setCurrent( -1);

        if (str.isEmpty())
            return false;
        else
        {
            if (item &&
                    Schema == item->text(2) &&
                    Object == item->text(0) &&
                    Type == item->text(3))
                setCurrent(item->text(1).toInt() - 1);

            return readErrors(conn);
        }
    }
    TOCATCH
    return false;
}

void toDebugText::setData(const QString &schema, const QString &type, const QString &object)
{
    Schema = schema;
    Type = type;
    Object = object;
    CurrentItem = FirstItem = NULL;
    NoBreakpoints = false;
    update();
}

toDebugText::toDebugText(QListView *breakpoints,
                         QWidget *parent,
                         toDebug *debugger)
        : toHighlightedText(parent, QString::number(++ID).latin1()),
        Debugger(debugger),
        Breakpoints(breakpoints)
{
    setLeftIgnore(DEBUG_INDENT);
    setHMargin(DEBUG_INDENT + hMargin());
    setMouseTracking(true);
    QRect view = viewRect();
    LastX = DEBUG_INDENT + view.left();
    CurrentItem = FirstItem = NULL;
    NoBreakpoints = false;
    if (!toBreakpointPixmap)
        toBreakpointPixmap = new QPixmap((const char **)breakpoint_xpm);
    if (!toDisBreakpointPixmap)
        toDisBreakpointPixmap = new QPixmap((const char **)disbreakpoint_xpm);
}

bool toDebugText::checkItem(toBreakpointItem *item)
{
    if (!item)
        return false;
    if (item->text(2) == Schema &&
            item->text(3) == Type &&
            item->text(0) == Object)
        return true;
    return false;
}

void toDebugText::clear(void)
{
    setData(QString::null, QString::null, QString::null);
    FirstItem = CurrentItem = NULL;
    NoBreakpoints = false;
    toHighlightedText::clear();
}

bool toDebugText::hasBreakpoint(int row) // This has to leave CurrentItem on the breakpoint
{
    if (!FirstItem && !NoBreakpoints)
    {
        FirstItem = dynamic_cast<toBreakpointItem *>(Breakpoints->firstChild());
        while (!checkItem(FirstItem) && FirstItem)
            FirstItem = dynamic_cast<toBreakpointItem *>(FirstItem->nextSibling());
        if (!FirstItem)
            NoBreakpoints = true;
        CurrentItem = FirstItem;
    }

    if (!NoBreakpoints)
    {
        toBreakpointItem *next = dynamic_cast<toBreakpointItem *>(CurrentItem->nextSibling());
        bool hasNext = checkItem(next);
        int nextLine = hasNext ? next->line() : row + 1;

        if (CurrentItem->line() == row)
            return true;
        if (row == nextLine)
        {
            CurrentItem = next;
            return true;
        }
        if (!hasNext && row > CurrentItem->line())
            return false;
        if (row < CurrentItem->line())
        {
            if (CurrentItem == FirstItem)
                return false;
            CurrentItem = FirstItem;
            return hasBreakpoint(row);
        }
        if (row > nextLine)
        {
            CurrentItem = next;
            return hasBreakpoint(row);
        }
    }
    return false;
}

void toDebugText::paintCell(QPainter *painter, int row, int col)
{
    toHighlightedText::paintCell(painter, row, col);
    if (col == 0)
    {
        QPalette cp = qApp->palette();
        painter->fillRect(0, 0, DEBUG_INDENT - 2, cellHeight(), cp.active().background());
        painter->fillRect(DEBUG_INDENT - 2, 0, 1, cellHeight(), cp.active().midlight());
        painter->fillRect(DEBUG_INDENT - 1, 0, 1, cellHeight(), cp.active().dark());

        if (hasBreakpoint(row))
        {
            int h = max((cellHeight() - toBreakpointPixmap->height()) / 2, 0);
            if (CurrentItem->text(4) == "DISABLED")
                painter->drawPixmap(0, h, *toDisBreakpointPixmap);
            else
                painter->drawPixmap(0, h, *toBreakpointPixmap);
        }
    }
}

void toDebugText::paintEvent(QPaintEvent *pe)
{
    toHighlightedText::paintEvent(pe);
    int fromY = 0;
    if (numRows() == 0 || rowYPos(numRows() - 1, &fromY))
    {
        if (numRows())
            fromY += cellHeight();
        QRect view = viewRect();
        if (fromY + view.top() < view.height())
        {
            QPainter painter(this);
            QPalette cp = qApp->palette();
            if (xOffset() < DEBUG_INDENT - 3)
                painter.fillRect(view.left(), fromY, DEBUG_INDENT - 2 - xOffset(), view.bottom() - fromY, cp.active().background());
            if (xOffset() < DEBUG_INDENT - 2)
                painter.fillRect(view.left() + DEBUG_INDENT - 2 - xOffset(), fromY, 1, view.bottom() - fromY, cp.active().midlight());
            if (xOffset() < DEBUG_INDENT - 1)
                painter.fillRect(view.left() + DEBUG_INDENT - 1 - xOffset(), fromY, 1, view.bottom() - fromY, cp.active().dark());
        }
    }
}

void toDebugText::mouseMoveEvent(QMouseEvent *me)
{
    QRect view = viewRect();
    if (me->x() > DEBUG_INDENT + view.left())
    {
        if (LastX <= DEBUG_INDENT + view.left())
            setCursor(Qt::ibeamCursor);
        if (me->state() != 0)
            toHighlightedText::mouseMoveEvent(me);
    }
    else
    {
        if (LastX > DEBUG_INDENT + view.left())
            setCursor(Qt::ibeamCursor);
        setCursor(Qt::arrowCursor);
    }
    LastX = me->x();
}

void toDebugText::toggleBreakpoint(int row, bool enable)
{
    if (Schema.isEmpty() ||
            Type.isEmpty() ||
            Object.isEmpty())
        return ;

    int curcol;
    if (row < 0)
        getCursorPosition (&row, &curcol);
    if (row >= 0)
    {
        if (hasBreakpoint(row))
        {
            if (enable)
            {
                if (CurrentItem->text(4) == qApp->translate("toDebug", "DISABLED"))
                    CurrentItem->setText(4, qApp->translate("toDebug", "DEFERED"));
                else
                    CurrentItem->clearBreakpoint();
            }
            else
            {
                CurrentItem->clearBreakpoint();
                delete CurrentItem;
                if (FirstItem == CurrentItem)
                {
                    NoBreakpoints = false;
                    CurrentItem = FirstItem = NULL;
                }
                else
                    CurrentItem = FirstItem;
            }
        }
        else if (!enable)
        {
            if (CurrentItem && CurrentItem->line() > row)
                new toBreakpointItem(Breakpoints, NULL,
                                     Schema, Type, Object, row);
            else
                new toBreakpointItem(Breakpoints, CurrentItem,
                                     Schema, Type, Object, row);
            FirstItem = CurrentItem = NULL;
            NoBreakpoints = false;
        }
        updateCell(row, 0, false);
    }
}

void toDebugText::mouseDoubleClickEvent(QMouseEvent *me)
{
    if (me->x() + xOffset() > DEBUG_INDENT)
        toHighlightedText::mouseDoubleClickEvent(me);
    else
    {
        int row = findRow(me->y());
        toggleBreakpoint(row);
    }
}

void toDebugText::exportData(std::map<QCString, QString> &data, const QCString &prefix)
{
    toHighlightedText::exportData(data, prefix);
    data[prefix + ":Schema"] = Schema;
    data[prefix + ":Object"] = Object;
    data[prefix + ":Type"] = Type;
}

void toDebugText::importData(std::map<QCString, QString> &data, const QCString &prefix)
{
    toHighlightedText::importData(data, prefix);
    Schema = data[prefix + ":Schema"];
    Object = data[prefix + ":Object"];
    Type = data[prefix + ":Type"];
    NoBreakpoints = false;
}
