/***************************************************************************
 *   Copyright (c) Eivind Kvedalen        (eivind@kvedalen.name) 2015      *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#	include <cassert>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Property.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "Expression.h"
#include <Base/Tools.h>

using namespace App;
using namespace Base;

std::size_t App::hash_value(const App::ObjectIdentifier & path)
{
    return boost::hash_value(path.toString());
}

// Path class

std::string App::quote(const std::string &input)
{
    std::stringstream output;

    std::string::const_iterator cur = input.begin();
    std::string::const_iterator end = input.end();

    output << "<<";
    while (cur != end) {
        switch (*cur) {
        case '\t':
            output << "\\t";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\'':
            output << "\\'";
            break;
        case '"':
            output << "\\\"";
            break;
        case '>':
            output << "\\>";
            break;
        default:
            output << *cur;
        }
        ++cur;
    }
    output << ">>";

    return output.str();
}

ObjectIdentifier::ObjectIdentifier(const App::PropertyContainer * _owner, const std::string & property)
    : owner(_owner)
    , documentNameSet(false)
    , documentObjectNameSet(false)
{
    if (property.size() > 0)
        addComponent(Component::SimpleComponent(property));
}

ObjectIdentifier::ObjectIdentifier(const Property &prop)
    : owner(prop.getContainer())
    , documentNameSet(false)
    , documentObjectNameSet(false)
{
    addComponent(Component::SimpleComponent(owner->getPropertyName(&prop)));
}

const std::string App::ObjectIdentifier::getPropertyName() const
{
    resolve();

    assert(propertyIndex >=0 && propertyIndex < components.size());

    return components[propertyIndex].toString();
}

const App::ObjectIdentifier::Component &App::ObjectIdentifier::getPropertyComponent(int i) const
{
    resolve();

    assert(propertyIndex + i >=0 && propertyIndex + i < components.size());

    return components[propertyIndex + i];
}

bool ObjectIdentifier::operator ==(const ObjectIdentifier &other) const
{
    resolve();
    other.resolve();

    if (owner != other.owner)
        return false;
    if (documentName != other.documentName)
        return false;
    if (documentObjectName != other.documentObjectName)
        return false;
    if (components != other.components)
        return false;
    return true;
}

bool ObjectIdentifier::operator !=(const ObjectIdentifier &other) const
{
    return !(operator==)(other);
}

bool ObjectIdentifier::operator <(const ObjectIdentifier &other) const
{
    resolve();
    other.resolve();

    if (documentName < other.documentName)
        return true;

    if (documentName > other.documentName)
        return false;

    if (documentObjectName < other.documentObjectName)
        return true;

    if (documentObjectName > other.documentObjectName)
        return false;

    if (components.size() < other.components.size())
        return true;

    if (components.size() > other.components.size())
        return false;

    for (int i = 0; i < components.size(); ++i) {
        if (components[i].name < other.components[i].name)
            return true;
        if (components[i].name > other.components[i].name)
            return false;
        if (components[i].type < other.components[i].type)
            return true;
        if (components[i].type > other.components[i].type)
            return false;
        if (components[i].isArray()) {
            if (components[i].index < other.components[i].index)
                return true;
            if (components[i].index > other.components[i].index)
                return false;
        }
        else if (components[i].isMap()) {
            if (components[i].key < other.components[i].key)
                return true;
            if (components[i].key > other.components[i].key)
                return false;
        }
    }
    return false;
}

int ObjectIdentifier::numComponents() const
{
    return components.size();
}

int ObjectIdentifier::numSubComponents() const
{
    return components.size() - propertyIndex;
}

std::string ObjectIdentifier::toString() const
{
    std::stringstream s;

    resolve();

    if (documentNameSet) {
        if (getDocumentName().isRealString())
            s << quote(getDocumentName().getString()) << "#";
        else
            s << getDocumentName().getString() << "#";
    }

    if (documentObjectNameSet) {
        if (getDocumentObjectName().isRealString())
            s << quote(getDocumentObjectName().getString()) << ".";
        else
            s << getDocumentObjectName().getString() << ".";
    }
    else if (propertyIndex > 0)
        s << components[0].name << ".";

    s << getPropertyName() << getSubPathStr();

    return s.str();
}

std::string ObjectIdentifier::toEscapedString() const
{
    return Base::Tools::escapedUnicodeFromUtf8(toString().c_str());
}

void ObjectIdentifier::renameDocumentObject(const std::string &oldName, const std::string &newName)
{
    resolve();

    if (documentObjectNameSet && documentObjectName == oldName) {
        documentObjectName = newName;
    }
    else if (propertyIndex == 1 && documentObjectName == oldName) {
        components[0].name = newName;
    }
}

void ObjectIdentifier::renameDocument(const std::string &oldName, const std::string &newName)
{
    resolve();

    if (documentName == oldName) {
        documentName = newName;
    }
}

std::string ObjectIdentifier::getSubPathStr() const
{
    resolve();

    std::stringstream s;
    std::vector<Component>::const_iterator i = components.begin() + propertyIndex + 1;
    while (i != components.end()) {
        s << "." << i->toString();
        ++i;
    }

    return s.str();
}

ObjectIdentifier::Component::Component(const std::string &_component, ObjectIdentifier::Component::typeEnum _type, int _index, String _key)
    : name(_component)
    , type(_type)
    , index(_index)
    , key(_key)
{
}

ObjectIdentifier::Component ObjectIdentifier::Component::SimpleComponent(const std::string &_component)
{
    return Component(_component);
}

ObjectIdentifier::Component ObjectIdentifier::Component::ArrayComponent(const std::string &_component, int _index)
{
    return Component(_component, ARRAY, _index);
}

ObjectIdentifier::Component ObjectIdentifier::Component::MapComponent(const std::string &_component, const String & _key)
{
    return Component(_component, MAP, -1, _key);
}

bool ObjectIdentifier::Component::operator ==(const ObjectIdentifier::Component &other) const
{
    if (type != other.type)
        return false;

    if (name != other.name)
        return false;

    switch (type) {
    case SIMPLE:
        return true;
    case ARRAY:
        return index == other.index;
    case MAP:
        return key == other.key;
    default:
        assert(0);
        return false;
    }
}

std::string ObjectIdentifier::Component::toString() const
{
    std::stringstream s;

    s << name;
    switch (type) {
    case Component::SIMPLE:
        break;
    case Component::MAP:
        s << "[" << key.toString() << "]";
        break;
    case Component::ARRAY:
        s << "[" << index << "]";
        break;
    default:
        assert(0);
    }

    return s.str();
}

App::DocumentObject * ObjectIdentifier::getDocumentObject(const App::Document * doc, const std::string & name) const
{
    DocumentObject * o1 = 0;
    DocumentObject * o2 = 0;
    std::vector<DocumentObject*> docObjects = doc->getObjects();

    for (std::vector<DocumentObject*>::iterator j = docObjects.begin(); j != docObjects.end(); ++j) {
        if (strcmp((*j)->Label.getValue(), name.c_str()) == 0) {
            // Found object with matching label
            if (o1 != 0)
                return 0;
            o1 = *j;
        }
    }

    // No object found with matching label, try using name directly
    o2 = doc->getObject(name.c_str());

    if (o1 == 0 && o2 == 0) // Not found at all
        return 0;
    else if (o1 == 0) // Found by name
        return o2;
    else if (o2 == 0) // Found by label
        return o1;
    else if (o1 == o2) // Found by both name and label, same object
        return o1;
    else
        return 0; // Found by both name and label, two different objects
}

void ObjectIdentifier::resolve() const
{
    const App::Document * doc;
    const App::DocumentObject * docObject;

    if (freecad_dynamic_cast<DocumentObject>(owner) == 0)
        return;

    /* Document name specified? */
    if (documentName.getString().size() > 0) {
        doc = getDocument(documentName);
    }
    else
        doc = freecad_dynamic_cast<DocumentObject>(owner)->getDocument();

    propertyName = "";
    propertyIndex = 0;

    // Assume document name and object name from owner if not found
    if (doc == 0) {
        doc = freecad_dynamic_cast<DocumentObject>(owner)->getDocument();
        if (doc == 0) {
            documentName = String();
            documentObjectName = String();
            return;
        }
    }

    documentName = String(doc->Label.getValue());

    /* Document object name specified? */
    if (documentObjectNameSet) {
        docObject = getDocumentObject(doc, documentObjectName.getString());
        if (!docObject)
            return;
        if (components.size() > 0) {
            propertyName = components[0].name;
            propertyIndex = 0;
        }
        else
            return;
    }
    else {
        /* Document object name not specified, resolve from path */
        if (components.size() == 1) {
            documentObjectName = String(freecad_dynamic_cast<DocumentObject>(owner)->getNameInDocument());
            propertyName = components[0].name;
            propertyIndex = 0;
        }
        else if (components.size() >= 2) {
            if (!components[0].isSimple())
                return;

            docObject = getDocumentObject(doc, components[0].name);

            if (docObject) {
                documentObjectName = components[0].name;
                propertyName = components[1].name;
                propertyIndex = 1;
            }
            else {
                documentObjectName = String(freecad_dynamic_cast<DocumentObject>(owner)->getNameInDocument());
                propertyName = components[0].name;
                propertyIndex = 0;
            }
        }
        else
            return;
    }
}

Document * ObjectIdentifier::getDocument(String name) const
{
    App::Document * doc = 0;
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();

    if (name.getString().size() == 0)
        name = getDocumentName();

    for (std::vector<App::Document*>::const_iterator i = docs.begin(); i != docs.end(); ++i) {
        if ((*i)->Label.getValue() == name.getString()) {
            if (doc != 0)
                return 0;
            doc = *i;
        }
    }

    return doc;
}

DocumentObject *ObjectIdentifier::getDocumentObject() const
{
    const App::Document * doc = getDocument();

    if (!doc)
        return 0;

    return  getDocumentObject(doc, documentObjectName);
}

std::vector<std::string> ObjectIdentifier::getStringList() const
{
    std::vector<std::string> l;

    if (documentNameSet)
        l.push_back(documentName.toString());
    if (documentObjectNameSet)
        l.push_back(documentObjectName.toString());

    std::vector<Component>::const_iterator i = components.begin();
    while (i != components.end()) {
        l.push_back(i->toString());
        ++i;
    }

    return l;
}

ObjectIdentifier ObjectIdentifier::parse(const DocumentObject *docObj, const std::string &str)
{
    std::auto_ptr<Expression> expr(ExpressionParser::parse(docObj, str.c_str()));
    VariableExpression * v = freecad_dynamic_cast<VariableExpression>(expr.get());

    if (v)
        return v->getPath();
    else
        throw Base::Exception("Invalid property specification.");
}

ObjectIdentifier &ObjectIdentifier::operator <<(const ObjectIdentifier::Component &value)
{
    components.push_back(value);
    return *this;
}

Property *ObjectIdentifier::getProperty() const
{
    const App::Document * doc = getDocument();

    if (!doc)
        return 0;

    App::DocumentObject * docObj = getDocumentObject(doc, documentObjectName);

    if (!docObj)
        return 0;

    return docObj->getPropertyByName(getPropertyComponent(0).getName().c_str());
}

ObjectIdentifier ObjectIdentifier::canonicalPath() const
{
    // Simplify input path by ensuring that components array only has property + optional sub-properties first.
    ObjectIdentifier simplified(getDocumentObject());

    for (int i = propertyIndex; i < components.size(); ++i)
        simplified << components[i];

    Property * prop = getProperty();

    // Invoke properties canonicalPath method, to let the property do the rest of the job.

    return prop ? prop->canonicalPath(simplified) : simplified;
}

void ObjectIdentifier::setDocumentName(const ObjectIdentifier::String &name, bool force)
{
    documentName = name;
    documentNameSet = force;
}

const ObjectIdentifier::String ObjectIdentifier::getDocumentName() const
{
    resolve();
    return documentName;
}

void ObjectIdentifier::setDocumentObjectName(const ObjectIdentifier::String &name, bool force)
{
    documentObjectName = name;
    documentObjectNameSet = force;
}

const ObjectIdentifier::String ObjectIdentifier::getDocumentObjectName() const
{
    resolve();
    return documentObjectName;
}

std::string ObjectIdentifier::String::toString() const
{
    if (isRealString())
        return quote(str);
    else
        return str;
}
