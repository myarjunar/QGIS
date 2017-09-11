/***************************************************************************
    qgsbrowserdockwidget.h
    ---------------------
    begin                : July 2011
    copyright            : (C) 2011 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSBROWSERDOCKWIDGET_H
#define QGSBROWSERDOCKWIDGET_H

#include "ui_qgsbrowserdockwidgetbase.h"
#include "ui_qgsbrowserlayerpropertiesbase.h"
#include "ui_qgsbrowserdirectorypropertiesbase.h"
#include "ui_qgsbrowserpropertiesdialogbase.h"

#include "qgsdataitem.h"
#include "qgsbrowsertreeview.h"
#include "qgsdockwidget.h"
#include "qgsbrowserdockwidget_p.h"
#include "qgis_gui.h"
#include "qgsbrowserdockwidgetdefaultactions.h"
#include <QSortFilterProxyModel>

class QgsBrowserModel;
class QModelIndex;
class QgsDockBrowserTreeView;
class QgsLayerItem;
class QgsDataItem;
class QgsBrowserTreeFilterProxyModel;
class QgsBrowserDockWidgetMenuProvider;
class QgsBrowserDockWidgetDefaultActions;

/**
 * \ingroup gui
 * The QgsBrowserDockWidget class
 * \since QGIS 3.0
 */
class GUI_EXPORT QgsBrowserDockWidget : public QgsDockWidget, private Ui::QgsBrowserDockWidgetBase
{
    Q_OBJECT
  public:

    /**
      * Constructor for QgsBrowserDockWidget
      * \param name name of the widget
      * \param parent parent widget
      */
    explicit QgsBrowserDockWidget( const QString &name, QWidget *parent SIP_TRANSFERTHIS = nullptr );
    ~QgsBrowserDockWidget();

    //! Get access to current data item
    QgsDataItem *currentDataItem() const ;

    //! Get access to the browser model
    QgsBrowserModel *browserModel() const { return mModel; }
    //! Get access to the browser proxy model
    QgsBrowserTreeFilterProxyModel *proxyModel() const { return mProxyModel; } SIP_SKIP
    //! Get access to the browser tree view
    QgsDockBrowserTreeView *browserView() const { return mBrowserView; } SIP_SKIP

    //! Get access to the default actions that may be used with the tree view
    QgsBrowserDockWidgetDefaultActions *defaultActions() SIP_SKIP;

    //! Set provider for context menu. Takes ownership of the instance
    void setMenuProvider( QgsBrowserDockWidgetMenuProvider *menuProvider SIP_TRANSFER );
    //! Return pointer to the context menu provider. May be null
    QgsBrowserDockWidgetMenuProvider *menuProvider() const { return mMenuProvider; } SIP_SKIP

  public slots:
    //! Add layer at index
    void addLayerAtIndex( const QModelIndex &index );
    //! Show context menu
    void showContextMenu( QPoint );

    //! Refresh browser view model (and view)
    void refresh();

    //! Show/hide filter widget
    void showFilterWidget( bool visible );
    //! Enable/disable properties widget
    void enablePropertiesWidget( bool enable );
    //! Set filter syntax
    void setFilterSyntax( QAction * );
    //! Set filter case sensitivity
    void setCaseSensitive( bool caseSensitive );
    //! Apply filter to the model
    void setFilter();
    //! Update project home directory
    void updateProjectHome();

    //! Add selected layers to the project
    void addSelectedLayers();

    //! Selection has changed
    void selectionChanged( const QItemSelection &selected, const QItemSelection &deselected );
    //! Splitter has been moved
    void splitterMoved();

  signals:
    //! Emitted when a file needs to be opened
    void openFile( const QString & );
    //! Emitted when drop uri list needs to be handled
    void handleDropUriList( const QgsMimeDataUtils::UriList & );
    //! Connections changed in the browser
    void connectionsChanged();

  protected:
    //! Show event override
    void showEvent( QShowEvent *event ) override;

  private:
    //! Refresh the model
    void refreshModel( const QModelIndex &index );
    //! Add a layer
    void addLayer( QgsLayerItem *layerItem );
    //! Clear the properties widget
    void clearPropertiesWidget();
    //! Set the properties widget
    void setPropertiesWidget();

    //! Count selected items
    int selectedItemsCount();
    //! Settings prefix (the object name)
    QString settingsSection() { return objectName().toLower(); }

  protected:
    //! helper class with default actions. Lazily initialized.
    QgsBrowserDockWidgetDefaultActions *mDefaultActions = nullptr;
    //! Context menu provider. Owned by the view.
    QgsBrowserDockWidgetMenuProvider *mMenuProvider = nullptr;
    QgsDockBrowserTreeView *mBrowserView = nullptr;
    QgsBrowserModel *mModel = nullptr;
    QgsBrowserTreeFilterProxyModel *mProxyModel = nullptr;
    QString mInitPath;
    bool mPropertiesWidgetEnabled;
    // height fraction
    float mPropertiesWidgetHeight;

};

/** \ingroup gui
 * Implementation of this interface can be implemented to allow QgsBrowserTreeView
 * instance to provide custom context menus (opened upon right-click).
 *
 * \see QgsBrowserTreeView
 * \since QGIS 3.0
 */
class GUI_EXPORT QgsBrowserDockWidgetMenuProvider
{
public:
  virtual ~QgsBrowserDockWidgetMenuProvider() = default;

  //! Return a newly created menu instance (or null pointer on error)
  virtual QMenu *createContextMenu( QgsDataItem *item ) = 0 SIP_FACTORY;
};

#endif // QGSBROWSERDOCKWIDGET_H
