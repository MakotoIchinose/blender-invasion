import bpy
from . base import FunctionNode, DataSocket
from . inferencer import Inferencer
from . socket_decl import FixedSocketDecl, ListSocketDecl, BaseSocketDecl, AnyVariadicDecl
from collections import defaultdict
from . sockets import type_infos, OperatorSocket, DataSocket

class TestOperator(bpy.types.Operator):
    bl_idname = "fn.test_operator"
    bl_label = "FN Test"

    def execute(self, context):
        tree = context.space_data.node_tree
        run_socket_operators(tree)
        run_socket_type_inferencer(tree)
        return {'FINISHED'}

def run_socket_operators(tree):
    while True:
        for link in tree.links:
            if isinstance(link.to_socket, OperatorSocket):
                node = link.to_node
                own_socket = link.to_socket
                other_socket = link.from_socket
                decl = link.to_node.storage.decl_per_socket[own_socket]
                tree.links.remove(link)
                decl.operator_socket_call(node, own_socket, other_socket)
                break
            elif isinstance(link.from_socket, OperatorSocket):
                node = link.from_node
                own_socket = link.from_socket
                other_socket = link.to_socket
                decl = node.storage.decl_per_socket[own_socket]
                tree.links.remove(link)
                decl.operator_socket_call(node, own_socket, other_socket)
                break
        else:
            return

def run_operator__extend_variadic(tree, node, decl, other_socket):
    if not isinstance(other_socket, DataSocket):
        return

    data_type = other_socket.data_type
    collection = getattr(node, decl.prop_name)
    item = collection.add()
    item.data_type = data_type

    node.rebuild_current_declaration__keep_state()
    target = node.storage.sockets_per_decl[decl][-2]
    tree.links.new(target, other_socket)

def run_socket_type_inferencer(tree):
    inferencer = Inferencer(type_infos)

    for node in tree.nodes:
        insert_constraints__within_node(inferencer, node)

    for link in tree.links:
        insert_constraints__link(inferencer, link)

    inferencer.inference()

    rebuild_nodes_and_links_that_changed(tree, inferencer)

def rebuild_nodes_and_links_that_changed(tree, inferencer):
    links_by_node = get_link_ids_by_node(tree)

    nodes_to_rebuild = set()
    links_to_rebuild = set()

    for (node, prop_name), value in inferencer.get_decisions().items():
        if getattr(node, prop_name) != value:
            setattr(node, prop_name, value)
            nodes_to_rebuild.add(node)

    for node in nodes_to_rebuild:
        links_to_rebuild.update(links_by_node.get(node))
        node.rebuild_current_declaration()

    for link_id in links_to_rebuild:
        from_socket = socket_by_id(link_id[0])
        to_socket = socket_by_id(link_id[1])
        tree.links.new(to_socket, from_socket)

def get_link_ids_by_node(tree):
    links_by_node = defaultdict(set)

    for link in tree.links:
        link_id = get_link_id(link)
        links_by_node[link.from_node].add(link_id)
        links_by_node[link.to_node].add(link_id)

    return links_by_node


# Insert inferencer constraints
########################################

def insert_constraints__within_node(inferencer, node):
    storage = node.storage

    list_ids_per_prop = defaultdict(set)
    base_ids_per_prop = defaultdict(set)

    for decl, sockets in storage.sockets_per_decl.items():
        if isinstance(decl, FixedSocketDecl):
            socket_id = get_socket_id_from_socket(node, sockets[0])
            inferencer.insert_final_type(socket_id, decl.data_type)
        elif isinstance(decl, ListSocketDecl):
            socket_id = get_socket_id_from_socket(node, sockets[0])
            list_ids_per_prop[decl.type_property].add(socket_id)
        elif isinstance(decl, BaseSocketDecl):
            socket_id = get_socket_id_from_socket(node, sockets[0])
            base_ids_per_prop[decl.type_property].add(socket_id)

    properties = set()
    properties.update(list_ids_per_prop.keys())
    properties.update(base_ids_per_prop.keys())

    for prop in properties:
        inferencer.insert_list_constraint(
            list_ids_per_prop[prop],
            base_ids_per_prop[prop],
            (node, prop))

def insert_constraints__link(inferencer, link):
    if not isinstance(link.from_socket, DataSocket):
        return
    if not isinstance(link.from_socket, DataSocket):
        return

    from_id = get_socket_id_from_socket(link.from_node, link.from_socket)
    to_id = get_socket_id_from_socket(link.to_node, link.to_socket)

    inferencer.insert_equality_constraint((from_id, to_id))


# Temporary IDs
########################################

def get_socket_id_from_socket(node, socket):
    index = get_socket_index(node, socket)
    return (node, socket.is_output, index)

def get_socket_id(node, is_output, i):
    return (node, is_output, i)

def get_socket_index(node, socket):
    if socket.is_output:
        return tuple(node.outputs).index(socket)
    else:
        return tuple(node.inputs).index(socket)

def get_link_id(link):
    from_id = get_socket_id_from_socket(link.from_node, link.from_socket)
    to_id = get_socket_id_from_socket(link.to_node, link.to_socket)
    return (from_id, to_id)

def socket_by_id(socket_id):
    node = socket_id[0]
    if socket_id[1]:
        return node.outputs[socket_id[2]]
    else:
        return node.inputs[socket_id[2]]