#include "PreCompiled.h"

#ifndef _PreComp_
#include <QStandardItem>
#include <QStandardItemModel>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include "ExpressionCompleter.h"

Q_DECLARE_METATYPE(App::ObjectIdentifier);

using namespace App;
using namespace Gui;

ExpressionCompleter::ExpressionCompleter(const App::Document * currentDoc, const App::DocumentObject * currentDocObj, QObject *parent)
    : QCompleter(parent)
{
    QStandardItemModel* model = new QStandardItemModel(this);

    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    std::vector<App::Document*>::const_iterator di = docs.begin();

    /* Create tree with full path to all objects */
    while (di != docs.end()) {
        QStandardItem* docItem = new QStandardItem(QString::fromAscii((*di)->getName()));

        docItem->setData(QString::fromAscii((*di)->getName()) + QString::fromAscii("#"), Qt::UserRole);
        createModelForDocument(*di, docItem);

        model->appendRow(docItem);

        ++di;
    }

    /* Create branch with current document object */

    if (currentDocObj) {
        createModelForDocument(currentDocObj->getDocument(), model->invisibleRootItem());
        createModelForDocumentObject(currentDocObj, model->invisibleRootItem());
    }
    else {
        if (currentDoc)
            createModelForDocument(currentDoc, model->invisibleRootItem());
    }

    setModel(model);

#ifdef FC_DEBUG
    setCompletionMode(InlineCompletion);
#endif
}

void ExpressionCompleter::createModelForDocument(const App::Document * doc, QStandardItem * parent) {
    std::vector<App::DocumentObject*> docObjs = doc->getObjects();
    std::vector<App::DocumentObject*>::const_iterator doi = docObjs.begin();

    while (doi != docObjs.end()) {
        QStandardItem* docObjItem = new QStandardItem(QString::fromAscii((*doi)->getNameInDocument()));

        docObjItem->setData(QString::fromAscii((*doi)->getNameInDocument()) + QString::fromAscii("."), Qt::UserRole);
        createModelForDocumentObject(*doi, docObjItem);
        parent->appendRow(docObjItem);

        if (strcmp((*doi)->getNameInDocument(), (*doi)->Label.getValue()) != 0) {
            docObjItem = new QStandardItem(QString::fromUtf8((*doi)->Label.getValue()));

            docObjItem->setData( QString::fromUtf8((*doi)->Label.getValue()) + QString::fromAscii("."), Qt::UserRole);
            createModelForDocumentObject(*doi, docObjItem);
            parent->appendRow(docObjItem);
        }

        ++doi;
    }
}

void ExpressionCompleter::createModelForDocumentObject(const DocumentObject * docObj, QStandardItem * parent)
{
    std::vector<App::Property*> props;
    docObj->getPropertyList(props);

    std::vector<App::Property*>::const_iterator pi = props.begin();
    while (pi != props.end()) {
        createModelForPaths(*pi, parent);
        ++pi;
    }
}

void ExpressionCompleter::createModelForPaths(const App::Property * prop, QStandardItem *docObjItem)
{
    std::vector<ObjectIdentifier> paths;
    std::vector<ObjectIdentifier>::const_iterator ppi;

    prop->getPaths(paths);

    for (ppi = paths.begin(); ppi != paths.end(); ++ppi) {
        QStandardItem* pathItem = new QStandardItem(QString::fromStdString(ppi->toString()));

        QVariant value;

        value.setValue(*ppi);
        pathItem->setData(value, Qt::UserRole);

        docObjItem->appendRow(pathItem);
    }
}

QString ExpressionCompleter::pathFromIndex ( const QModelIndex & index ) const
{
    QStandardItemModel * m = static_cast<QStandardItemModel*>(model());

    if (m->data(index, Qt::UserRole).canConvert<App::ObjectIdentifier>()) {
        App::ObjectIdentifier p = m->data(index, Qt::UserRole).value<App::ObjectIdentifier>();
        QString pStr = QString::fromStdString(p.toString());

        QString parentStr;
        QModelIndex parent = index.parent();
        while (parent.isValid()) {
            QString thisParentStr = m->data(parent, Qt::UserRole).toString();

            parentStr = thisParentStr + parentStr;

            parent = parent.parent();
        }

        return parentStr + pStr;
    }
    else if (m->data(index, Qt::UserRole).canConvert<QString>()) {
        QModelIndex parent = index;
        QString parentStr;

        while (parent.isValid()) {
            QString thisParentStr = m->data(parent, Qt::UserRole).toString();

            parentStr = thisParentStr + parentStr;

            parent = parent.parent();
        }

        return parentStr;
    }
    else
        return QString();
}

QStringList ExpressionCompleter::splitPath ( const QString & path ) const
{
    try {
        App::ObjectIdentifier p = ObjectIdentifier::parse(0, path.toStdString());
        QStringList l;

        if (p.getProperty()) {
            for (int i = 0; i < p.numComponents(); ++i)
                l << QString::fromStdString(p.getPropertyComponent(i).toString());
            return l;
        }
        else {
            std::vector<std::string> sl = p.getStringList();
            std::vector<std::string>::const_iterator sli = sl.begin();

            while (sli != sl.end()) {
                l << QString::fromStdString(*sli);
                ++sli;
            }

            return l;
        }
    }
    catch (const Base::Exception &) {
        return QStringList() << path;
    }
}

