#include "PreCompiled.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include "Expression.h"
#include "PropertyExpressionEngine.h"
#include "PropertyStandard.h"
#include "PropertyUnits.h"
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>


using namespace App;
using namespace Base;
using namespace boost;

TYPESYSTEM_SOURCE(App::PropertyExpressionEngine , App::Property);

PropertyExpressionEngine::PropertyExpressionEngine()
    : Property()
    , running(false)
    , validator(0)
{
}

PropertyExpressionEngine::~PropertyExpressionEngine()
{
}

unsigned int PropertyExpressionEngine::getMemSize() const
{
    return 0;
}

Property *PropertyExpressionEngine::Copy() const
{
    PropertyExpressionEngine * engine = new PropertyExpressionEngine();

    for (ExpressionMap::const_iterator it = expressions.begin(); it != expressions.end(); ++it)
        engine->expressions[it->first] = ExpressionInfo(it->second);

    engine->disabledExpressions = disabledExpressions;
    engine->validator = validator;

    return engine;
}

void PropertyExpressionEngine::Paste(const Property &from)
{
    const PropertyExpressionEngine * fromee = static_cast<const PropertyExpressionEngine*>(&from);

    aboutToSetValue();
    expressions.clear();

    for (ExpressionMap::const_iterator it = fromee->expressions.begin(); it != fromee->expressions.end(); ++it)
        expressions[it->first] = ExpressionInfo(it->second);

    disabledExpressions = fromee->disabledExpressions;
    validator = fromee->validator;

    hasSetValue();
}

void PropertyExpressionEngine::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<ExpressionEngine count=\"" <<  expressions.size() <<"\">" << std::endl;
    writer.incInd();
    for (ExpressionMap::const_iterator it = expressions.begin(); it != expressions.end(); ++it) {
        writer.Stream() << writer.ind() << "<Expression path=\"" <<  Property::encodeAttribute(it->first.toString()) <<"\"" <<
                           " expression=\"" << Property::encodeAttribute(it->second.expression->toString()) << "\"";
        if (it->second.comment.size() > 0)
            writer.Stream() << " comment=\"" << Property::encodeAttribute(it->second.comment) << "\"";
        writer.Stream() << "/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</ExpressionEngine>" << std::endl;
}

void PropertyExpressionEngine::Restore(Base::XMLReader &reader)
{
    reader.readElement("ExpressionEngine");

    int count = reader.getAttributeAsFloat("count");

    for (int i = 0; i < count; ++i) {
        DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(getContainer());

        reader.readElement("Expression");
        ObjectIdentifier path = ObjectIdentifier::parse(docObj, reader.getAttribute("path"));
        boost::shared_ptr<Expression> expression(ExpressionParser::parse(docObj, reader.getAttribute("expression")));
        const char * comment = reader.hasAttribute("comment") ? reader.getAttribute("comment") : 0;

        setValue(path, expression, comment);
    }

    reader.readEndElement("ExpressionEngine");
}

void PropertyExpressionEngine::buildGraphStructures(const ObjectIdentifier & path,
                                                    const boost::shared_ptr<Expression> expression,
                                                    boost::unordered_map<ObjectIdentifier, int> & nodes,
                                                    boost::unordered_map<int, ObjectIdentifier> & revNodes,
                                                    std::vector<Edge> & edges) const
{
    std::set<ObjectIdentifier> deps;

    /* Insert target property into nodes structure */
    if (nodes.find(path) == nodes.end()) {
        int s = nodes.size();

        revNodes[s] = path;
        nodes[path] = s;
    }
    else
        revNodes[nodes[path]] = path;

    /* Get the dependencies for this expression */
    expression->getDeps(deps);

    /* Insert dependencies into nodes structure */
    std::set<ObjectIdentifier>::const_iterator di = deps.begin();
    while (di != deps.end()) {
        Property * prop = di->getProperty();

        if (prop) {
            ObjectIdentifier cPath(di->canonicalPath());

            if (nodes.find(cPath) == nodes.end()) {
                int s = nodes.size();

                nodes[cPath] = s;
            }

            edges.push_back(std::make_pair(nodes[path], nodes[cPath]));
        }
        ++di;
    }
}

const ObjectIdentifier PropertyExpressionEngine::canonicalPath(const ObjectIdentifier &p) const
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(getContainer());

    // Am I owned by a DocumentObject?
    if (!docObj)
        throw Base::Exception("PropertyExpressionEngine must be owned by a DocumentObject.");

    Property * prop = p.getProperty();

    // p pointing to a property...?
    if (!prop)
        throw Base::Exception("Property not found");

    // ... in the same container as I?
    if (prop->getContainer() != getContainer())
        throw Base::Exception("Property does not belong to same container as PropertyExpressionEngine");

    // In case someone calls this with p pointing to a PropertyExpressionEngine for some reason
    if (prop->isDerivedFrom(PropertyExpressionEngine::classTypeId))
        return p;

    // Dispatch call to actual canonicalPath implementation
    return p.canonicalPath();
}

const boost::any PropertyExpressionEngine::getValue(const App::ObjectIdentifier & path) const
{
    // Get a canonical path
    ObjectIdentifier usePath(canonicalPath(path));

    ExpressionMap::const_iterator i = expressions.find(usePath);

    if (i != expressions.end())
        return i->second;
    else
        return boost::any();
}

void PropertyExpressionEngine::setValue(const ObjectIdentifier & path, boost::shared_ptr<Expression> expr, const char *comment)
{
    ObjectIdentifier usePath(canonicalPath(path));
    const Property * prop = usePath.getProperty();

    // Try to access value; it should trigger an exception if it is not supported, or if the path is invalid
    prop->getValue(usePath);

    if (expr) {
        std::string error = validateExpression(usePath, expr);

        if (error.size() > 0)
            throw Base::Exception(error.c_str());

        aboutToSetValue();
        expressions[usePath] = ExpressionInfo(expr, comment);
        hasSetValue();
    }
    else {
        aboutToSetValue();
        expressions.erase(usePath);
        hasSetValue();
    }
}

struct cycle_detector : public boost::dfs_visitor<> {
    cycle_detector( bool& has_cycle, int & src)
      : _has_cycle(has_cycle), _src(src) { }

    template <class Edge, class Graph>
    void back_edge(Edge e, Graph&g) {
      _has_cycle = true;
      _src = source(e, g);
    }

  protected:
    bool& _has_cycle;
    int & _src;
};

void PropertyExpressionEngine::buildGraph(const ExpressionMap & exprs,
                                          boost::unordered_map<int, ObjectIdentifier> & revNodes, DiGraph & g) const
{
    boost::unordered_map<ObjectIdentifier, int> nodes;
    std::vector<Edge> edges;

    // Build data structure for graph
    for (ExpressionMap::const_iterator it = exprs.begin(); it != exprs.end(); ++it)
        buildGraphStructures(it->first, it->second.expression, nodes, revNodes, edges);

    // Create graph
    g = DiGraph(revNodes.size());

    // Add edges to graph
    for (std::vector<Edge>::const_iterator i = edges.begin(); i != edges.end(); ++i)
        add_edge(i->first, i->second, g);

    // Check for cycles
    bool has_cycle = false;
    int src = -1;
    cycle_detector vis(has_cycle, src);
    depth_first_search(g, visitor(vis));

    if (has_cycle) {
        std::string s =  revNodes[src].toString() + " reference creates a cyclic dependency.";

        throw Base::Exception(s.c_str());
    }
}

/**
 * The code below builds a graph for all expressions in the engine, and
 * finds any circular dependencies. It also computes the internal evaluation
 * order, in case properties depends on each other.
 */

std::vector<App::ObjectIdentifier> PropertyExpressionEngine::computeEvaluationOrder()
{
    std::vector<App::ObjectIdentifier> evaluationOrder;
    boost::unordered_map<int, ObjectIdentifier> revNodes;
    DiGraph g;

    buildGraph(expressions, revNodes, g);

    /* Compute evaluation order for expressions */
    std::vector<int> c;
    topological_sort(g, std::back_inserter(c));

    for (std::vector<int>::iterator i = c.begin(); i != c.end(); ++i) {
        if (revNodes.find(*i) != revNodes.end())
            evaluationOrder.push_back(revNodes[*i]);
    }

    return evaluationOrder;
}

DocumentObjectExecReturn *App::PropertyExpressionEngine::execute()
{
    DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(getContainer());

    if (!docObj)
        throw Base::Exception("PropertyExpressionEngine must be owned by a DocumentObject.");

    if (running)
        return DocumentObject::StdReturn;

    /* Resetter class, to ensure that the "running" variable gets set to false, even if
     * an exception is thrown.
     */

    class resetter {
    public:
        resetter(bool & b) : _b(b) { _b = true; }
        ~resetter() { _b = false; }

    private:
        bool & _b;
    };

    resetter r(running);

    // Compute evaluation order
    std::vector<App::ObjectIdentifier> evaluationOrder = computeEvaluationOrder();
    std::vector<ObjectIdentifier>::const_iterator it = evaluationOrder.begin();

#ifdef FC_PROPERTYEXPRESSIONENGINE_LOG
    std::clog << "Computing expressions for " << getName() << std::endl;
#endif

    /* Evaluate the expressions, and update properties */
    while (it != evaluationOrder.end()) {

        /* Skip disabled expressions */
        if (disabledExpressions.find(*it) != disabledExpressions.end()) {
            ++it;
            continue;
        }

        // Get property to update
        Property * prop = it->getProperty();

        if (!prop)
            throw Base::Exception("Path does not resolve to a property.");

        DocumentObject* parent = freecad_dynamic_cast<DocumentObject>(prop->getContainer());

        /* Make sure property belongs to the same container as this PropertyExpressionEngine */
        if (parent != docObj)
            throw Base::Exception("Invalid property owner.");

        // Evaluate expression
        std::auto_ptr<Expression> e(expressions[*it].expression->eval());

#ifdef FC_PROPERTYEXPRESSIONENGINE_LOG
        {
            Base::Quantity q;
            boost::any value = e->getValueAsAny();

            if (value.type() == typeid(Base::Quantity))
                q = boost::any_cast<Base::Quantity>(value);
            else if (value.type() == typeid(double))
                q = boost::any_cast<double>(value);
            else {
                std::clog << "Unknown return value for expression.";
                q = 0;
            }

            std::clog << "Assigning value " << q.getValue() << " to " << (*it).toString().c_str() << " (" << prop->getName() <<  ")" << std::endl;
        }
#endif

        /* Set value of property */
        prop->setValue(*it, e->getValueAsAny());

        ++it;
    }
    return DocumentObject::StdReturn;
}

void PropertyExpressionEngine::getDocumentObjectDeps(std::vector<DocumentObject *> &docObjs) const
{
    DocumentObject * owner = freecad_dynamic_cast<DocumentObject>(getContainer());

    if (owner == 0)
        return;

    ExpressionMap::const_iterator i = expressions.begin();

    while (i != expressions.end()) {
        std::set<ObjectIdentifier> deps;

        i->second.expression->getDeps(deps);

        std::set<ObjectIdentifier>::const_iterator j = deps.begin();

        while (j != deps.end()) {
            const ObjectIdentifier & p = *j;
            DocumentObject* docObj = p.getDocumentObject();

            if (docObj && docObj != owner)
                docObjs.push_back(docObj);

            ++j;
        }
        ++i;
    }
}

bool PropertyExpressionEngine::depsAreTouched() const
{
    ExpressionMap::const_iterator i = expressions.begin();

    while (i != expressions.end()) {
        std::set<ObjectIdentifier> deps;

        i->second.expression->getDeps(deps);

        std::set<ObjectIdentifier>::const_iterator j = deps.begin();

        while (j != deps.end()) {
            const ObjectIdentifier & p = *j;
            Property* prop = p.getProperty();

            if (prop && prop->isTouched())
                return true;

            ++j;
        }
        ++i;
    }

    return false;
}

boost::unordered_map<const ObjectIdentifier, const PropertyExpressionEngine::ExpressionInfo> PropertyExpressionEngine::getExpressions() const
{
    boost::unordered_map<const ObjectIdentifier, const ExpressionInfo> result;

    ExpressionMap::const_iterator i = expressions.begin();
    while (i != expressions.end()) {
        result.insert(std::make_pair(i->first, i->second));
        ++i;
    }

    return result;
}

std::string PropertyExpressionEngine::validateExpression(const ObjectIdentifier &path, boost::shared_ptr<const Expression> expr) const
{
    std::string error;
    ObjectIdentifier usePath(canonicalPath(path));

    if (validator) {
        error = validator(usePath, expr);
        if (error.size() > 0)
            return error;
    }

    // Get dependencies from expression
    std::set<App::ObjectIdentifier> exprDeps;
    expr->getDeps(exprDeps);

    // Get document object
    DocumentObject * pathDocObj = usePath.getDocumentObject();

    // Check for document object dependecies
    for (std::set<App::ObjectIdentifier>::const_iterator j = exprDeps.begin(); j != exprDeps.end(); ++j) {
        DocumentObject * docObj = (*j).getDocumentObject();

        // Skip internal dependencies;
        if (docObj == pathDocObj)
            continue;

        // Get dependencies for the document object pointed to be *j
        std::vector<DocumentObject*> targets;
        targets.push_back(docObj);
        std::vector<DocumentObject*> deps = (*j).getDocument()->getDependencyList(targets);

        for (std::vector<DocumentObject*>::const_iterator i = deps.begin(); i != deps.end(); ++i) {
            if (*i == pathDocObj)
                return (*j).toString() + " reference creates a cyclic dependency.";
        }
    }

    // Check for internal document object dependencies

    // Copy current expressions
    ExpressionMap newExpressions = expressions;

    // Add expression in question
    boost::shared_ptr<Expression> exprClone(expr->copy());
    newExpressions[usePath].expression = exprClone;

    // Build graph; an exception will be thrown if it is not a DAG
    try {
        boost::unordered_map<int, ObjectIdentifier> revNodes;
        DiGraph g;

        buildGraph(newExpressions, revNodes, g);
    }
    catch (const Base::Exception & e) {
        return e.what();
    }

    return std::string();
}

void PropertyExpressionEngine::renameExpressions(const std::map<ObjectIdentifier, ObjectIdentifier> & paths)
{
    ExpressionMap newExpressions;
    std::map<ObjectIdentifier, ObjectIdentifier> canonicalPaths;

    /* ensure input map uses canonical paths */
    for (std::map<ObjectIdentifier, ObjectIdentifier>::const_iterator i = paths.begin(); i != paths.end(); ++i)
        canonicalPaths[canonicalPath(i->first)] = i->second;

    for (ExpressionMap::const_iterator i = expressions.begin(); i != expressions.end(); ++i) {
        std::map<ObjectIdentifier, ObjectIdentifier>::const_iterator j = canonicalPaths.find(i->first);

        // Renamed now?
        if (j != canonicalPaths.end())
            newExpressions[j->second] = i->second;
        else
            newExpressions[i->first] = i->second;
    }

    aboutToSetValue();
    expressions = newExpressions;
    hasSetValue();
}

std::set<ObjectIdentifier> PropertyExpressionEngine::getPaths() const
{
    ExpressionMap::const_iterator i = expressions.begin();
    std::set<ObjectIdentifier> result;

    while (i != expressions.end()) {
        result.insert(i->first);
        ++i;
    }

    return result;
}
