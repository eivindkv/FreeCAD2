#ifndef EXPRESSIONENGINE_H
#define EXPRESSIONENGINE_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/function.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <App/Property.h>
#include <App/Expression.h>
#include <set>

namespace Base {
class Writer;
class XMLReader;
}

namespace App {

class DocumentObject;
class DocumentObjectExecReturn;
class ObjectIdentifier;
class Expression;

class PropertyExpressionEngine : public App::Property
{
    TYPESYSTEM_HEADER();
public:

    typedef boost::function<std::string (const App::ObjectIdentifier & path, boost::shared_ptr<const App::Expression> expr)> ValidatorFunc;

    struct ExpressionInfo {
        boost::shared_ptr<App::Expression> expression;
        std::string comment;

        ExpressionInfo(boost::shared_ptr<App::Expression> expression = boost::shared_ptr<App::Expression>(), const char * comment = 0) {
            this->expression = expression;
            if (comment)
                this->comment = comment;
        }

        ExpressionInfo(const ExpressionInfo & other) {
            expression = other.expression;
            comment = other.comment;
        }

        ExpressionInfo & operator=(const ExpressionInfo & other) {
            expression = other.expression;
            comment = other.comment;
            return *this;
        }
    };

    PropertyExpressionEngine();
    ~PropertyExpressionEngine();

    unsigned int getMemSize (void) const;

    void setValue() { } // Dummy

    Property *Copy(void) const;

    void Paste(const Property &from);

    void Save (Base::Writer & writer) const;

    void Restore(Base::XMLReader &reader);

    void setValue(const App::ObjectIdentifier &path, boost::shared_ptr<App::Expression> expr, const char * comment = 0);

    const boost::any getValue(const App::ObjectIdentifier & path) const;

    DocumentObjectExecReturn * execute();

    void getDocumentObjectDeps(std::vector<DocumentObject*> & docObjs) const;

    bool depsAreTouched() const;

    boost::unordered_map<const App::ObjectIdentifier, const ExpressionInfo> getExpressions() const;

    /* Expression validator */
    void setValidator(ValidatorFunc f) { validator = f; }

    std::string validateExpression(const App::ObjectIdentifier & path, boost::shared_ptr<const App::Expression> expr) const;

    void renameExpressions(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths);

    std::set<App::ObjectIdentifier> getPaths() const;

    const App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier &p) const;

private:

    typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS > DiGraph;
    typedef std::pair<int, int> Edge;
    typedef boost::unordered_map<const App::ObjectIdentifier, ExpressionInfo> ExpressionMap;

    std::vector<App::ObjectIdentifier> computeEvaluationOrder();

    void buildGraphStructures(const App::ObjectIdentifier &path,
                              const boost::shared_ptr<Expression> expression, boost::unordered_map<App::ObjectIdentifier, int> &nodes,
                              boost::unordered_map<int, App::ObjectIdentifier> &revNodes, std::vector<Edge> &edges) const;

    void buildGraph(const ExpressionMap &exprs,
                    boost::unordered_map<int, App::ObjectIdentifier> &revNodes, DiGraph &g) const;

    bool running;

    ExpressionMap expressions; /**< Stored expressions */

    boost::unordered_map<const App::ObjectIdentifier, int> disabledExpressions; /**< Disabled expressions; negative numbers => ignored */

    ValidatorFunc validator;

};

}

#endif // EXPRESSIONENGINE_H
