#pragma once

#include "field.h"

#include "wdissector.h"

#include <memory>
#include <vector>

struct Field_Node {
    Field_Node() = default;
    Field_Node(std::shared_ptr<Field> f, std::weak_ptr<Field_Node> parent) : field(f), parent(parent) {}

    inline void add_child(std::shared_ptr<Field_Node> child_node) {
        children.push_back(child_node);
    }

    inline std::vector<std::shared_ptr<Field_Node>> get_children() {
        return children;
    }

    std::shared_ptr<Field> field;
    std::weak_ptr<Field_Node> parent;
    std::vector<std::shared_ptr<Field_Node>> children;

    std::ostream& dump(std::ostream& os) const;
};

class Field_Tree {
public:
    void add_field(std::shared_ptr<Field> f);
    std::ostream& dump(std::ostream& os) const;

    inline int get_field_tree_offset() const {
        return head->children.front()->field->get_offset();
    }

    inline int get_field_tree_end() const {
        return head->children.back()->field->get_offset_end();
    }

    inline std::vector<std::shared_ptr<Field_Node>> get_field_nodes_mut() {
        return field_nodes;
    }

    inline const std::vector<std::shared_ptr<Field_Node>>& get_field_nodes() const {
        return field_nodes;
    }

    inline const std::vector<std::shared_ptr<Field_Node>>& get_group_field_nodes() const {
        return group_field_nodes;
    }

    inline const std::vector<std::shared_ptr<Field_Node>>& get_layer_field_nodes() const {
        return layer_field_nodes;
    }

private:
    std::shared_ptr<Field_Node> head = std::make_shared<Field_Node>();
    std::weak_ptr<Field_Node> cur_node = head;
    std::vector<std::shared_ptr<Field_Node>> field_nodes; // Vector of pointers to all the fields (type FIELD) 
    std::vector<std::shared_ptr<Field_Node>> group_field_nodes; // Vector of pointers to all the group fields (type GROUP_FIELD) 
    std::vector<std::shared_ptr<Field_Node>> layer_field_nodes; // Vector of pointers to all the layer fields (type LAYER_FIELD) 
};

std::ostream& operator<<(std::ostream& os, const Field_Tree& f);
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Field_Tree>& f);

std::ostream& operator<<(std::ostream& os, const Field_Node& f);
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Field_Node>& f);