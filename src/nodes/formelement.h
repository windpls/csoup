#ifndef CSOUP_FORM_ELEMENT_H_
#define CSOUP_FORM_ELEMENT_H_

#include "element.h"

namespace csoup {
    class ElementsRef;
    
    class FormElement : public Element {
    public:
        FormElement(const StringRef& tagName, const StringRef& baseUri, Allocator* allocator) :
        Element(CSOUP_NODE_FORMELEMENT, tagName, baseUri, allocator), elements_(NULL) {
            
        }
        
        FormElement(const StringRef& tagName, const Attributes& attributes, const StringRef& baseUri, Allocator* allocator) :
        Element(CSOUP_NODE_FORMELEMENT, tagName, attributes, baseUri, allocator), elements_(NULL) {
            
        }
        
        ~FormElement();
        
        ElementsRef* elements() {
            return elements_;
        }
        
        const ElementsRef* elements() const {
            return elements_;
        }
        
        void appendElementToForm(Element* ele);
    private:
        ElementsRef* ensureElementsRef();
        
        ElementsRef* elements_;
    };
}

#endif