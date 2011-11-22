/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "itemviews.h"

#include <qheaderview.h>
#include <qtableview.h>
#include <qlistview.h>
#include <qtreeview.h>
#include <private/qtreewidget_p.h>
#include <qaccessible2.h>
#include <QDebug>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);

#ifndef QT_NO_ITEMVIEWS
/*
Implementation of the IAccessible2 table2 interface. Much simpler than
the other table interfaces since there is only the main table and cells:

TABLE/LIST/TREE
  |- HEADER CELL
  |- CELL
  |- CELL
  ...
*/

int QAccessibleTable::logicalIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return -1;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    // row * number columns + column + 1 for one based counting
    return (index.row() + hHeader)*(index.model()->columnCount() + vHeader) + (index.column() + vHeader) + 1;
}

QAccessibleInterface *QAccessibleTable::childFromLogical(int logicalIndex) const
{
    logicalIndex--; // one based counting ftw
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;

    int columns = view->model()->columnCount() + vHeader;

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    if (vHeader) {
        if (column == 0) {
            if (row == 0) {
                return new QAccessibleTableCornerButton(view);
            }
            return new QAccessibleTableHeaderCell(view, row-1, Qt::Vertical);
        }
        --column;
    }
    if (hHeader) {
        if (row == 0) {
            return new QAccessibleTableHeaderCell(view, column, Qt::Horizontal);
        }
        --row;
    }
    return new QAccessibleTableCell(view, view->model()->index(row, column), cellRole());
}

QAccessibleTable::QAccessibleTable(QWidget *w)
    : QAccessibleObject(w)
{
    view = qobject_cast<QAbstractItemView *>(w);
    Q_ASSERT(view);

    if (qobject_cast<const QTableView*>(view)) {
        m_role = QAccessible::Table;
    } else if (qobject_cast<const QTreeView*>(view)) {
        m_role = QAccessible::Tree;
    } else if (qobject_cast<const QListView*>(view)) {
        m_role = QAccessible::List;
    } else {
        // is this our best guess?
        m_role = QAccessible::Table;
    }
}

QAccessibleTable::~QAccessibleTable()
{
}

QHeaderView *QAccessibleTable::horizontalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->horizontalHeader();
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }
    return header;
}

QHeaderView *QAccessibleTable::verticalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->verticalHeader();
#endif
    }
    return header;
}

void QAccessibleTable::modelReset()
{}

void QAccessibleTable::rowsInserted(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = first;
    lastChange.lastRow = last;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeInsert;
}

void QAccessibleTable::rowsRemoved(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = first;
    lastChange.lastRow = last;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeDelete;
}

void QAccessibleTable::columnsInserted(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = first;
    lastChange.lastColumn = last;
    lastChange.type = QAccessible2::TableModelChangeInsert;
}

void QAccessibleTable::columnsRemoved(const QModelIndex &, int first, int last)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = first;
    lastChange.lastColumn = last;
    lastChange.type = QAccessible2::TableModelChangeDelete;
}

void QAccessibleTable::rowsMoved( const QModelIndex &, int, int, const QModelIndex &, int)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeUpdate;
}

void QAccessibleTable::columnsMoved( const QModelIndex &, int, int, const QModelIndex &, int)
{
    lastChange.firstRow = 0;
    lastChange.lastRow = 0;
    lastChange.firstColumn = 0;
    lastChange.lastColumn = 0;
    lastChange.type = QAccessible2::TableModelChangeUpdate;
}

QAccessibleTableCell *QAccessibleTable::cell(const QModelIndex &index) const
{
    if (index.isValid())
        return new QAccessibleTableCell(view, index, cellRole());
    return 0;
}

QAccessibleInterface *QAccessibleTable::cellAt(int row, int column) const
{
    Q_ASSERT(role() != QAccessible::Tree);
    QModelIndex index = view->model()->index(row, column);
    if (!index.isValid()) {
        qWarning() << "QAccessibleTable::cellAt: invalid index: " << index << " for " << view;
        return 0;
    }
    return cell(index);
}

QAccessibleInterface *QAccessibleTable::caption() const
{
    return 0;
}

QString QAccessibleTable::columnDescription(int column) const
{
    return view->model()->headerData(column, Qt::Horizontal).toString();
}

int QAccessibleTable::columnCount() const
{
    return view->model()->columnCount();
}

int QAccessibleTable::rowCount() const
{
    return view->model()->rowCount();
}

int QAccessibleTable::selectedCellCount() const
{
    return view->selectionModel()->selectedIndexes().count();
}

int QAccessibleTable::selectedColumnCount() const
{
    return view->selectionModel()->selectedColumns().count();
}

int QAccessibleTable::selectedRowCount() const
{
    return view->selectionModel()->selectedRows().count();
}

QString QAccessibleTable::rowDescription(int row) const
{
    return view->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleInterface *> QAccessibleTable::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    Q_FOREACH (const QModelIndex &index, view->selectionModel()->selectedIndexes()) {
        cells.append(cell(index));
    }
    return cells;
}

QList<int> QAccessibleTable::selectedColumns() const
{
    QList<int> columns;
    Q_FOREACH (const QModelIndex &index, view->selectionModel()->selectedColumns()) {
        columns.append(index.column());
    }
    return columns;
}

QList<int> QAccessibleTable::selectedRows() const
{
    QList<int> rows;
    Q_FOREACH (const QModelIndex &index, view->selectionModel()->selectedRows()) {
        rows.append(index.row());
    }
    return rows;
}

QAccessibleInterface *QAccessibleTable::summary() const
{
    return 0;
}

bool QAccessibleTable::isColumnSelected(int column) const
{
    return view->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool QAccessibleTable::isRowSelected(int row) const
{
    return view->selectionModel()->isRowSelected(row, QModelIndex());
}

bool QAccessibleTable::selectRow(int row)
{
    QModelIndex index = view->model()->index(row, 0);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Select);
    return true;
}

bool QAccessibleTable::selectColumn(int column)
{
    QModelIndex index = view->model()->index(0, column);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Select);
    return true;
}

bool QAccessibleTable::unselectRow(int row)
{
    QModelIndex index = view->model()->index(row, 0);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Deselect);
    return true;
}

bool QAccessibleTable::unselectColumn(int column)
{
    QModelIndex index = view->model()->index(0, column);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Columns & QItemSelectionModel::Deselect);
    return true;
}

QAccessible2::TableModelChange QAccessibleTable::modelChange() const
{
    QAccessible2::TableModelChange change;
    // FIXME
    return change;
}

QAccessible::Role QAccessibleTable::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTable::state() const
{
    return QAccessible::Normal;
}

QAccessibleInterface *QAccessibleTable::childAt(int x, int y) const
{
    QPoint viewportOffset = view->viewport()->mapTo(view, QPoint(0,0));
    QPoint indexPosition = view->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view->indexAt(indexPosition);
    if (index.isValid()) {
        return childFromLogical(logicalIndex(index));
    }
    return 0;
}

int QAccessibleTable::childCount() const
{
    if (!view->model())
        return 0;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (view->model()->rowCount()+hHeader) * (view->model()->columnCount()+vHeader);
}

int QAccessibleTable::indexOfChild(const QAccessibleInterface *iface) const
{
    Q_ASSERT(iface->role() != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const QAccessibleTableCell* cell = static_cast<const QAccessibleTableCell*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return cell->index + (verticalHeader() ? 1 : 0) + 1;
    } else if (iface->role() == QAccessible::RowHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        return (cell->index+1) * (view->model()->rowCount()+1)  + 1;
    } else if (iface->role() == QAccessible::Pane) {
        return 1; // corner button
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild Fix my children..."
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString QAccessibleTable::text(QAccessible::Text t) const
{
    if (t == QAccessible::Description)
        return view->accessibleDescription();
    return view->accessibleName();
}

QRect QAccessibleTable::rect() const
{
    if (!view->isVisible())
        return QRect();
    QPoint pos = view->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view->width(), view->height());
}

QAccessibleInterface *QAccessibleTable::parent() const
{
    if (view->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view->parent());
    }
    return 0;
}

QAccessibleInterface *QAccessibleTable::child(int index) const
{
    // Fixme: get rid of the +1 madness
    return childFromLogical(index + 1);
}

int QAccessibleTable::navigate(QAccessible::RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    *iface = 0;
    switch (relation) {
    case QAccessible::Ancestor: {
        *iface = parent();
        return *iface ? 0 : -1;
    }
    case QAccessible::Child: {
        Q_ASSERT(index > 0);
        *iface = child(index - 1);
        if (*iface) {
            return 0;
        }
        break;
    }
    default:
        break;
    }
    return -1;
}

QAccessible::Relation QAccessibleTable::relationTo(const QAccessibleInterface *) const
{
    return QAccessible::Unrelated;
}

void *QAccessibleTable::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableInterface)
       return static_cast<QAccessibleTableInterface*>(this);
   return 0;
}

// TREE VIEW

QModelIndex QAccessibleTree::indexFromLogical(int row, int column) const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    QModelIndex modelIndex = treeView->d_func()->viewItems.at(row).index;

    if (modelIndex.isValid() && column > 0) {
        modelIndex = view->model()->index(modelIndex.row(), column, modelIndex.parent());
    }
    return modelIndex;
}

QAccessibleInterface *QAccessibleTree::childAt(int x, int y) const
{
    QPoint viewportOffset = view->viewport()->mapTo(view, QPoint(0,0));
    QPoint indexPosition = view->mapFromGlobal(QPoint(x, y) - viewportOffset);

    QModelIndex index = view->indexAt(indexPosition);
    if (!index.isValid())
        return 0;

    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    int row = treeView->d_func()->viewIndex(index) + (horizontalHeader() ? 1 : 0);
    int column = index.column();

    int i = row * view->model()->columnCount() + column + 1;
    Q_ASSERT(i > view->model()->columnCount());
    return child(i - 1);
}

int QAccessibleTree::childCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    Q_ASSERT(treeView);
    if (!view->model())
        return 0;

    int hHeader = horizontalHeader() ? 1 : 0;
    return (treeView->d_func()->viewItems.count() + hHeader)* view->model()->columnCount();
}

int QAccessibleTree::rowCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
    Q_ASSERT(treeView);
    return treeView->d_func()->viewItems.count();
}

int QAccessibleTree::indexOfChild(const QAccessibleInterface *iface) const
{
     if (iface->role() == QAccessible::TreeItem) {
        const QAccessibleTableCell* cell = static_cast<const QAccessibleTableCell*>(iface);
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        Q_ASSERT(treeView);
        int row = treeView->d_func()->viewIndex(cell->m_index) + (horizontalHeader() ? 1 : 0);
        int column = cell->m_index.column();

        int index = row * view->model()->columnCount() + column + 1;
        //qDebug() << "QAccessibleTree::indexOfChild r " << row << " c " << column << "index " << index;
        Q_ASSERT(index > treeView->model()->columnCount());
        return index;
    } else if (iface->role() == QAccessible::ColumnHeader){
        const QAccessibleTableHeaderCell* cell = static_cast<const QAccessibleTableHeaderCell*>(iface);
        //qDebug() << "QAccessibleTree::indexOfChild header " << cell->index << "is: " << cell->index + 1;
        return cell->index + 1;
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild invalid child"
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: add scrollbars and don't just ignore them
    return -1;
}

int QAccessibleTree::navigate(QAccessible::RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    switch (relation) {
    case QAccessible::Child: {
        Q_ASSERT(index > 0);
        --index;
        int hHeader = horizontalHeader() ? 1 : 0;

        if (hHeader) {
            if (index < view->model()->columnCount()) {
                *iface = new QAccessibleTableHeaderCell(view, index, Qt::Horizontal);
                return 0;
            } else {
                index -= view->model()->columnCount();
            }
        }

        int row = index / view->model()->columnCount();
        int column = index % view->model()->columnCount();
        QModelIndex modelIndex = indexFromLogical(row, column);
        if (modelIndex.isValid()) {
            *iface = cell(modelIndex);
            return 0;
        }
        return -1;
    }
    default:
        break;
    }
    return QAccessibleTable::navigate(relation, index, iface);
}

QAccessible::Relation QAccessibleTree::relationTo(const QAccessibleInterface *) const
{
    return QAccessible::Unrelated;
}

QAccessibleInterface *QAccessibleTree::cellAt(int row, int column) const
{
    QModelIndex index = indexFromLogical(row, column);
    if (!index.isValid()) {
        qWarning() << "Requested invalid tree cell: " << row << column;
        return 0;
    }
    return new QAccessibleTableCell(view, index, cellRole());
}

QString QAccessibleTree::rowDescription(int) const
{
    return QString(); // no headers for rows in trees
}

bool QAccessibleTree::isRowSelected(int row) const
{
    QModelIndex index = indexFromLogical(row);
    return view->selectionModel()->isRowSelected(index.row(), index.parent());
}

bool QAccessibleTree::selectRow(int row)
{
    QModelIndex index = indexFromLogical(row);
    if (!index.isValid() || view->selectionMode() & QAbstractItemView::NoSelection)
        return false;
    view->selectionModel()->select(index, QItemSelectionModel::Select);
    return true;
}

// TABLE CELL

QAccessibleTableCell::QAccessibleTableCell(QAbstractItemView *view_, const QModelIndex &index_, QAccessible::Role role_)
    : /* QAccessibleSimpleEditableTextInterface(this), */ view(view_), m_index(index_), m_role(role_)
{
    if (!index_.isValid())
        qWarning() << "QAccessibleTableCell::QAccessibleTableCell with invalid index: " << index_;
}

void *QAccessibleTableCell::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    return 0;
}

int QAccessibleTableCell::columnExtent() const { return 1; }
int QAccessibleTableCell::rowExtent() const { return 1; }

QList<QAccessibleInterface*> QAccessibleTableCell::rowHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (verticalHeader()) {
        headerCell.append(new QAccessibleTableHeaderCell(view, m_index.row(), Qt::Vertical));
    }
    return headerCell;
}

QList<QAccessibleInterface*> QAccessibleTableCell::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (horizontalHeader()) {
        headerCell.append(new QAccessibleTableHeaderCell(view, m_index.column(), Qt::Horizontal));
    }
    return headerCell;
}

QHeaderView *QAccessibleTableCell::horizontalHeader() const
{
    QHeaderView *header = 0;

    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->horizontalHeader();
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }

    return header;
}

QHeaderView *QAccessibleTableCell::verticalHeader() const
{
    QHeaderView *header = 0;
#ifndef QT_NO_TABLEVIEW
    if (const QTableView *tv = qobject_cast<const QTableView*>(view))
        header = tv->verticalHeader();
#endif
    return header;
}

int QAccessibleTableCell::columnIndex() const
{
    return m_index.column();
}

int QAccessibleTableCell::rowIndex() const
{
    if (role() == QAccessible::TreeItem) {
       const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
       Q_ASSERT(treeView);
       int row = treeView->d_func()->viewIndex(m_index);
       return row;
    }
    return m_index.row();
}

bool QAccessibleTableCell::isSelected() const
{
    return view->selectionModel()->isSelected(m_index);
}

void QAccessibleTableCell::rowColumnExtents(int *row, int *column, int *rowExtents, int *columnExtents, bool *selected) const
{
    *row = m_index.row();
    *column = m_index.column();
    *rowExtents = 1;
    *columnExtents = 1;
    *selected = isSelected();
}

QAccessibleInterface *QAccessibleTableCell::table() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessible::Role QAccessibleTableCell::role() const
{
    return m_role;
}

QAccessible::State QAccessibleTableCell::state() const
{
    QAccessible::State st = QAccessible::Normal;
    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    if (!globalRect.intersects(rect()))
        st |= QAccessible::Invisible;

    if (view->selectionModel()->isSelected(m_index))
        st |= QAccessible::Selected;
    if (view->selectionModel()->currentIndex() == m_index)
        st |= QAccessible::Focused;
    if (m_index.model()->data(m_index, Qt::CheckStateRole).toInt() == Qt::Checked)
        st |= QAccessible::Checked;

    Qt::ItemFlags flags = m_index.flags();
    if (flags & Qt::ItemIsSelectable) {
        st |= QAccessible::Selectable;
        st |= QAccessible::Focusable;
        if (view->selectionMode() == QAbstractItemView::MultiSelection)
            st |= QAccessible::MultiSelectable;
        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            st |= QAccessible::ExtSelectable;
    }
    if (m_role == QAccessible::TreeItem) {
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        if (treeView->model()->hasChildren(m_index))
            st |= QAccessible::Expandable;
        if (treeView->isExpanded(m_index))
            st |= QAccessible::Expanded;
    }
    return st;
}


QRect QAccessibleTableCell::rect() const
{
    QRect r;
    r = view->visualRect(m_index);

    if (!r.isNull())
        r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    return r;
}

QString QAccessibleTableCell::text(QAccessible::Text t) const
{
    QAbstractItemModel *model = view->model();
    QString value;
    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name:
        value = model->data(m_index, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->data(m_index, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->data(m_index, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void QAccessibleTableCell::setText(QAccessible::Text /*t*/, const QString &text)
{
    if (!(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool QAccessibleTableCell::isValid() const
{
    return m_index.isValid();
}

QAccessibleInterface *QAccessibleTableCell::parent() const
{
    if (m_role == QAccessible::TreeItem)
        return new QAccessibleTree(view);

    return new QAccessibleTable(view);
}

QAccessibleInterface *QAccessibleTableCell::child(int) const
{
    return 0;
}

int QAccessibleTableCell::navigate(QAccessible::RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    if (relation == QAccessible::Ancestor && index == 1) {
        *iface = parent();
        return 0;
    }

    *iface = 0;
    if (!view)
        return -1;

    switch (relation) {

    case QAccessible::Child: {
        return -1;
    }
    case QAccessible::Sibling:
        if (index > 0) {
            QAccessibleInterface *parent = QAccessible::queryAccessibleInterface(view);
            *iface = parent->child(index - 1);
            delete parent;
            return *iface ? 0 : -1;
        }
        return -1;

// From table1 implementation:
//    case Up:
//    case Down:
//    case Left:
//    case Right: {
//        // This is in the "not so nice" category. In order to find out which item
//        // is geometrically around, we have to set the current index, navigate
//        // and restore the index as well as the old selection
//        view->setUpdatesEnabled(false);
//        const QModelIndex oldIdx = view->currentIndex();
//        QList<QModelIndex> kids = children();
//        const QModelIndex currentIndex = index ? kids.at(index - 1) : QModelIndex(row);
//        const QItemSelection oldSelection = view->selectionModel()->selection();
//        view->setCurrentIndex(currentIndex);
//        const QModelIndex idx = view->moveCursor(toCursorAction(relation), Qt::NoModifier);
//        view->setCurrentIndex(oldIdx);
//        view->selectionModel()->select(oldSelection, QItemSelectionModel::ClearAndSelect);
//        view->setUpdatesEnabled(true);
//        if (!idx.isValid())
//            return -1;

//        if (idx.parent() != row.parent() || idx.row() != row.row())
//            *iface = cell(idx);
//        return index ? kids.indexOf(idx) + 1 : 0; }
    default:
        break;
    }

    return -1;
}

QAccessible::Relation QAccessibleTableCell::relationTo(const QAccessibleInterface *other) const
{
    // we only check for parent-child relationships in trees
    if (m_role == QAccessible::TreeItem && other->role() == QAccessible::TreeItem) {
        QModelIndex otherIndex = static_cast<const QAccessibleTableCell*>(other)->m_index;
        // is the other our parent?
        if (otherIndex.parent() == m_index)
            return QAccessible::Ancestor;
        // are we the other's child?
        if (m_index.parent() == otherIndex)
            return QAccessible::Child;
    }
    return QAccessible::Unrelated;
}

QAccessibleTableHeaderCell::QAccessibleTableHeaderCell(QAbstractItemView *view_, int index_, Qt::Orientation orientation_)
    : view(view_), index(index_), orientation(orientation_)
{
    Q_ASSERT(index_ >= 0);
}

QAccessible::Role QAccessibleTableHeaderCell::role() const
{
    if (orientation == Qt::Horizontal)
        return QAccessible::ColumnHeader;
    return QAccessible::RowHeader;
}

QAccessible::State QAccessibleTableHeaderCell::state() const
{
    return QAccessible::Normal;
}

QRect QAccessibleTableHeaderCell::rect() const
{
    QHeaderView *header = 0;
    if (false) {
#ifndef QT_NO_TABLEVIEW
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        if (orientation == Qt::Horizontal) {
            header = tv->horizontalHeader();
        } else {
            header = tv->verticalHeader();
        }
#endif
#ifndef QT_NO_TREEVIEW
    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
#endif
    }
    QPoint zero = header->mapToGlobal(QPoint(0, 0));
    int sectionSize = header->sectionSize(index);
    int sectionPos = header->sectionPosition(index);
    return orientation == Qt::Horizontal
            ? QRect(zero.x() + sectionPos, zero.y(), sectionSize, header->height())
            : QRect(zero.x(), zero.y() + sectionPos, header->width(), sectionSize);
}

QString QAccessibleTableHeaderCell::text(QAccessible::Text t) const
{
    QAbstractItemModel *model = view->model();
    QString value;
    switch (t) {
    case QAccessible::Value:
    case QAccessible::Name:
        value = model->headerData(index, orientation, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->headerData(index, orientation, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->headerData(index, orientation, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void QAccessibleTableHeaderCell::setText(QAccessible::Text, const QString &)
{
    return;
}

bool QAccessibleTableHeaderCell::isValid() const
{
    return true;
}

QAccessibleInterface *QAccessibleTableHeaderCell::parent() const
{
    if (false) {
#ifndef QT_NO_TREEVIEW
    } else if (qobject_cast<const QTreeView*>(view)) {
        return new QAccessibleTree(view);
#endif
    } else {
        return new QAccessibleTable(view);
    }
}

QAccessibleInterface *QAccessibleTableHeaderCell::child(int) const
{
    return 0;
}

int QAccessibleTableHeaderCell::navigate(QAccessible::RelationFlag relation, int index, QAccessibleInterface **iface) const
{
    if (relation == QAccessible::Ancestor && index == 1) {
        *iface = parent();
        return *iface ? 0 : -1;
    }
    *iface = 0;
    return -1;
}

QAccessible::Relation QAccessibleTableHeaderCell::relationTo(const QAccessibleInterface *) const
{
    return QAccessible::Unrelated;
}

#endif // QT_NO_ITEMVIEWS

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
