#ifndef EXPRESSIONCOMPLETER_H
#define EXPRESSIONCOMPLETER_H

#include <QObject>
#include <QCompleter>

class QStandardItem;

namespace App {
class Document;
class DocumentObject;
class Property;
class ObjectIdentifier;
}

namespace Gui {

class GuiExport ExpressionCompleter : public QCompleter
{
public:
    ExpressionCompleter(const App::Document * currentDoc, const App::DocumentObject * currentDocObj, QObject *parent = 0);

private:
    void createModelForDocument(const App::Document * doc, QStandardItem * parent);
    void createModelForDocumentObject(const App::DocumentObject * docObj, QStandardItem * parent);
    void createModelForPaths(const App::Property * prop, QStandardItem *docObjItem);

    virtual QString pathFromIndex ( const QModelIndex & index ) const;
    virtual QStringList splitPath ( const QString & path ) const;
};

}

#endif // EXPRESSIONCOMPLETER_H
