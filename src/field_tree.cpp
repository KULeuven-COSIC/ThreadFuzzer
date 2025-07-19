#include "field_tree.h"

#include "helpers.h"

/* Field_Node */
std::ostream& operator<<(std::ostream& os, const Field_Node& f) {
    return f.dump(os);
}

std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Field_Node>& f) {
    return f->dump(os);
}

std::ostream& Field_Node::dump(std::ostream& os) const {
    if (field) {
        std::string color = CRESET;
        switch(field->field_type) {
            case(FIELD_TYPE::LAYER):
                color = YELLOW;
                break;
            case(FIELD_TYPE::GROUP):
                color = BLUE;
                break;
            default:
                break;
        }
        os << color << "Name: " << field->field_name << ":" << field->parsed_f << CRESET;
    } else {
        os << RED "HEAD" CRESET << std::endl;
    }

    for (auto child : children) {
        os << child << std::endl;
    }
    return os;
}

/* Field_Tree */
void Field_Tree::add_field(std::shared_ptr<Field> f) {

    /* 1. Move to the correct node */
    while ((cur_node.lock())->field && (f->get_offset() >= (cur_node.lock())->field->get_offset_end())) {
        cur_node = (cur_node.lock())->parent;
    }

    /* 2. Add a children */
    std::shared_ptr<Field_Node> new_child = std::make_shared<Field_Node>(f, cur_node);
    (cur_node.lock())->add_child(new_child);

    switch(f->field_type) {
        case(FIELD_TYPE::FIELD):
            field_nodes.push_back(new_child);
            break;
        case(FIELD_TYPE::GROUP):
            group_field_nodes.push_back(new_child);
            cur_node = new_child;
            break;
        case(FIELD_TYPE::LAYER):
            layer_field_nodes.push_back(new_child);
            cur_node = new_child;
            break;
        default:
            break;
    }

}

std::ostream& operator<<(std::ostream& os, const Field_Tree& t) {
    return t.dump(os);
}

std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Field_Tree>& t) {
    return t->dump(os);
}

std::ostream& Field_Tree::dump(std::ostream& os) const {
    os << "Field Tree: " << std::endl;
    os << head << std::endl;
    return os;
}
