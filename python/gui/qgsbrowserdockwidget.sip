/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/gui/qgsbrowserdockwidget.h                                       *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/




class QgsBrowserDockWidget : QgsDockWidget
{
%Docstring
 The QgsBrowserDockWidget class
.. versionadded:: 3.0
%End

%TypeHeaderCode
#include "qgsbrowserdockwidget.h"
%End
  public:

    explicit QgsBrowserDockWidget( const QString &name, QWidget *parent /TransferThis/ = 0 );
%Docstring
 Constructor for QgsBrowserDockWidget
 \param name name of the widget
 \param parent parent widget
%End
    ~QgsBrowserDockWidget();

    QgsBrowserModel *browserModel() const;
%Docstring
Get access to the browser model
 :rtype: QgsBrowserModel
%End


    void setMenuProvider( QgsBrowserDockWidgetMenuProvider *menuProvider /Transfer/ );
%Docstring
Set provider for context menu. Takes ownership of the instance
%End

  public slots:
    void addLayerAtIndex( const QModelIndex &index );
%Docstring
Add layer at index
%End
    void showContextMenu( QPoint );
%Docstring
Show context menu
%End

    void refresh();
%Docstring
Refresh browser view model (and view)
%End

    void showFilterWidget( bool visible );
%Docstring
Show/hide filter widget
%End
    void enablePropertiesWidget( bool enable );
%Docstring
Enable/disable properties widget
%End
    void setFilterSyntax( QAction * );
%Docstring
Set filter syntax
%End
    void setCaseSensitive( bool caseSensitive );
%Docstring
Set filter case sensitivity
%End
    void setFilter();
%Docstring
Apply filter to the model
%End
    void updateProjectHome();
%Docstring
Update project home directory
%End

    void addSelectedLayers();
%Docstring
Add selected layers to the project
%End

    void selectionChanged( const QItemSelection &selected, const QItemSelection &deselected );
%Docstring
Selection has changed
%End
    void splitterMoved();
%Docstring
Splitter has been moved
%End

  signals:
    void openFile( const QString & );
%Docstring
Emitted when a file needs to be opened
%End
    void handleDropUriList( const QgsMimeDataUtils::UriList & );
%Docstring
Emitted when drop uri list needs to be handled
%End
    void connectionsChanged();
%Docstring
Connections changed in the browser
%End

  protected:
    virtual void showEvent( QShowEvent *event );

%Docstring
Show event override
%End

  protected:

};

class QgsBrowserDockWidgetMenuProvider
{
%Docstring
 Implementation of this interface can be implemented to allow QgsBrowserTreeView
 instance to provide custom context menus (opened upon right-click).

.. seealso:: QgsBrowserTreeView
.. versionadded:: 3.0
%End

%TypeHeaderCode
#include "qgsbrowserdockwidget.h"
%End
public:
  virtual ~QgsBrowserDockWidgetMenuProvider();

  virtual QMenu *createContextMenu( QgsDataItem *item ) = 0 /Factory/;
%Docstring
Return a newly created menu instance (or null pointer on error)
 :rtype: QMenu
%End
};

/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/gui/qgsbrowserdockwidget.h                                       *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/
