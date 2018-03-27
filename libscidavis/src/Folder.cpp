/***************************************************************************
    File                 : Folder.cpp
    Project              : SciDAVis
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Benkert,
					  Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
                           knut.franke*gmx.de
    Description          : Folder for the project explorer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "Folder.h"

#include <qobject.h>
#include <qdatetime.h>
#include <qpixmap.h>

#include <qevent.h>
#include <qpoint.h>
#include <qmessagebox.h>
#include <qmime.h>
#include <qstringlist.h>
#include <qapplication.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QDropEvent>
#include <QMouseEvent>

Folder::Folder( Folder *parent, const QString &name )
	: QObject(parent), d_active_window(0)
{
	QObject::setObjectName(name);
	birthdate = QDateTime::currentDateTime ().toString(Qt::LocalDate);

	// FIXME: This doesn't work anymore in Qt4, need alternative method
	// lstWindows.setAutoDelete( true );
}

QList<Folder*> Folder::folders() const
{
	QList<Folder*> lst;
	foreach(QObject *f, children())
		lst.append((Folder*) f);
	return lst;
}

QStringList Folder::subfolders()
{
	QStringList list = QStringList();
	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject * f;
		foreach(f,folderList)
			list << static_cast<Folder *>(f)->name();
	}
	return list;
}

QString Folder::path()
{
    QString s = "/" + QString(name()) + "/";
    Folder *parentFolder = (Folder *)parent();
    while (parentFolder){
        s.prepend("/" + QString(parentFolder->name()));
        parentFolder = (Folder *)parentFolder->parent();
	}
    return s;
}

Folder* Folder::findSubfolder(const QString& s, bool caseSensitive, bool partialMatch)
{
	QObjectList folderList = children();
	if (!folderList.isEmpty()){
		QObject * f;

		foreach(f,folderList){
			QString name = static_cast<Folder *>(f)->name();
			if (partialMatch){
				if (caseSensitive && name.startsWith(s,Qt::CaseSensitive))
					return static_cast<Folder *>(f);
				else if (!caseSensitive && name.startsWith(s,Qt::CaseInsensitive))
					return static_cast<Folder *>(f);
			}
			else // partialMatch == false
			{
				if (caseSensitive && name == s)
					return static_cast<Folder *>(f);
				else if ( !caseSensitive && (name.toLower() == s.toLower()) )
					return static_cast<Folder *>(f);
			}
		}
	}
	return 0;
}

MyWidget* Folder::findWindow(const QString& s, bool windowNames, bool labels,
							 bool caseSensitive, bool partialMatch)
{
	Qt::CaseSensitivity cs = Qt::CaseSensitive;
	if (!caseSensitive)
		cs = Qt::CaseInsensitive;
	MyWidget* w;
	foreach(w,lstWindows)
	{
		if (windowNames)
		{
			QString name = w->name();
			if (partialMatch && name.startsWith(s, cs))
				return w;
			else if (caseSensitive && name == s)
				return w;
			else
			{
				QString text = s;
				if (name == text.toLower())
					return w;
			}
		}

		if (labels)
		{
			QString label = w->windowLabel();
			if (partialMatch && label.startsWith(s, cs))
				return w;
			else if (caseSensitive && label == s)
				return w;
			else
			{
				QString text = s;
				if (label == text.toLower())
					return w;
			}
		}
	}
	return 0;
}

MyWidget *Folder::window(const QString &name, const char *cls, bool recursive)
{
	foreach (MyWidget *w, lstWindows)
		if (w->inherits(cls) && name == w->name().mid(0,w->name().indexOf("@")))
			return w;
	if (!recursive) return NULL;
	foreach (QObject *f, children())
	{
		MyWidget *w = ((Folder*)f)->window(name, cls, true);
		if (w) return w;
	}
	return NULL;
}

Folder* Folder::rootFolder()
{
	Folder *i = this;
	while(i->parent())
		i = (Folder*)i->parent();
	return i;
}

/*****************************************************************************
 *
 * Class FolderListItem
 *
 *****************************************************************************/

FolderListItem::FolderListItem( QTreeWidget *parent, Folder *f )
    : QTreeWidgetItem( parent )
{
    myFolder = f;

    setText( 0, f->name() );
	setOpen( true );
	setActive( true );
	setDragEnabled ( true );
	setDropEnabled ( true );
}

FolderListItem::FolderListItem( FolderListItem *parent, Folder *f )
    : QTreeWidgetItem( parent )
{
    myFolder = f;

    setText( 0, f->name() );
	setOpen( true );
	setActive( true );
}

void FolderListItem::setActive( bool o )
{
    if ( o )
		setPixmap(0, QPixmap(":/folder_open.xpm") );
    else
		setPixmap(0, QPixmap(":/folder_closed.xpm") );

	setSelected(o);
}

bool FolderListItem::isChildOf(FolderListItem *src)
{
FolderListItem *parent = (FolderListItem *)this->parent();
while (parent)
	{
	if (parent == src)
		return true;

	parent = (FolderListItem *)parent->parent();
	}
return false;
}

/*****************************************************************************
 *
 * Class FolderListView
 *
 *****************************************************************************/

FolderListView::FolderListView( QWidget *parent, const QString name )
	: QTreeWidget( parent, name.toLocal8Bit().constData() ), mousePressed( false )
{
    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
}

void FolderListView::startDrag()
{
QTreeWidgetItem *item = currentItem();
if (!item)
	return;

if (item == firstChild() && item->treeWidget()->rootIsDecorated())
	return;//it's the project folder so we don't want the user to move it

viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );

QPixmap pix;
if (item->rtti() == FolderListItem::RTTI)
	pix = QPixmap(":/folder_closed.xpm");
else
	pix = *item->pixmap (0);

QDrag *drag = new QDrag(viewport());
drag->setPixmap(pix, QPoint(pix.width()/2, pix.height()/2 ) );

QList<QTreeWidgetItem *> lst;
for (item = firstChild(); item; item = item->itemBelow())
	{
	if (item->isSelected())
		lst.append(item);
	}

emit dragItems(lst);
drag->drag();
}

void FolderListView::contentsDropEvent( QDropEvent *e )
{
QTreeWidgetItem *dest = itemAt( contentsToViewport(e->pos()) );
if ( dest && dest->rtti() == FolderListItem::RTTI)
	{
	emit dropItems(dest);
	e->accept();
    }
else
	e->ignore();
}

void FolderListView::keyPressEvent ( QKeyEvent * e )
{
if (isRenaming())
	{
	e->ignore();
	return;
	}
QTreeWidgetItem *item = currentItem();
if (!item) {
	QTreeWidget::keyPressEvent ( e );
	return;
}

if (item->rtti() == FolderListItem::RTTI &&
	(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return))
	{
	emit doubleClicked(item);
	e->accept();
	}
else if (e->key() == Qt::Key_F2)
	{
	if (item)
		emit renameItem(item);
	e->accept();
	}
else if(e->key() == Qt::Key_A && e->state() == Qt::ControlModifier)
	{
	selectAll(true);
	e->accept();
	}
else if(e->key() == Qt::Key_F7)
	{
	emit addFolderItem();
	e->accept();
	}
else if(e->key() == Qt::Key_F8)
	{
	emit deleteSelection();
	e->accept();
	}
else
	QTreeWidget::keyPressEvent ( e );
}

void FolderListView::contentsMouseDoubleClickEvent( QMouseEvent* e )
{
	if (isRenaming())
		{
		e->ignore();
		return;
		}

	QTreeWidget::contentsMouseDoubleClickEvent( e );
}

void FolderListView::contentsMousePressEvent( QMouseEvent* e )
{
QTreeWidget::contentsMousePressEvent(e);
if (e->button() != Qt::LeftButton) return;
QPoint p( contentsToViewport( e->pos() ) );
QTreeWidgetItem *i = itemAt( p );

if ( i )
		{// if the user clicked into the root decoration of the item, don't try to start a drag!
		if ( p.x() > header()->cellPos( header()->mapToActual( 0 ) ) +
			treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin() ||
			p.x() < header()->cellPos( header()->mapToActual( 0 ) ) )
			{
			presspos = e->pos();
	    	mousePressed = true;
			}
    	}
}

void FolderListView::contentsMouseMoveEvent( QMouseEvent* e )
{
if ( mousePressed && ( presspos - e->pos() ).manhattanLength() > QApplication::startDragDistance() )
	{
	mousePressed = false;
	QTreeWidgetItem *item = itemAt( contentsToViewport(presspos) );
	if ( item )
		startDrag();
    }
}

void FolderListView::adjustColumns()
{
for (int i=0; i < columns (); i++)
	adjustColumn(i);
}

/*****************************************************************************
 *
 * Class WindowListItem
 *
 *****************************************************************************/

WindowListItem::WindowListItem( QTreeWidget *parent, MyWidget *w )
    : QTreeWidgetItem( parent )
{
    myWindow = w;

	setDragEnabled ( true );
}
