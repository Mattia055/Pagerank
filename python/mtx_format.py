#! /usr/bin/env python3
import os


def check_if_zero(file_path):
    with open(file_path, 'r') as file:
        for line in file:
            if line.strip():
                parts = line.split()
                if len(parts) == 2:
                    if(int(parts[0]) == 0 or int(parts[1]) == 0):
                        return True
    return False


def increment_node_ids(file_path):
    """Increment all node IDs in the file by 1."""
    temp_file_path = file_path + '.tmp'
    
    with open(file_path, 'r') as infile, open(temp_file_path, 'w') as outfile:
        for line in infile:
            if line.strip(): 
                parts = line.split()
                if len(parts) == 2:  
                    try:
                        node1 = int(parts[0]) + 1
                        node2 = int(parts[1]) + 1
                        outfile.write(f"{node1} {node2}\n")
                    except ValueError:
                        continue  

    os.replace(temp_file_path, file_path)


def get_max_node_and_edge_count(file_path):
    """Find the maximum node ID and count the total number of edges in the file."""
    max_node_id = 0
    edge_count = 0
    with open(file_path, 'r') as file:
        for line in file:
            if line.strip():  
                parts = line.split()
                if len(parts) == 2:  
                    try:
                        node1 = int(parts[0])
                        node2 = int(parts[1])
                        max_node_id = max(max_node_id, node1, node2)
                        edge_count += 1
                    except ValueError:
                        continue  
    return max_node_id, edge_count

def add_header_to_file(file_path, num_nodes, num_edges):
    """Add a header line with the number of nodes and edges to the file."""
    with open(file_path, 'r') as file:
        original_content = file.read()
    
    header_line = f"{num_nodes} {num_nodes} {num_edges}\n"
    new_content = header_line + original_content
    
    with open(file_path, 'w') as file:
        file.write(new_content)

def process_folder(folder_path):
    """Process each file in the folder by adding a header line."""
    for filename in os.listdir(folder_path):
        file_path = os.path.join(folder_path, filename)
        if os.path.isfile(file_path):
            if(check_if_zero(file_path)):
                increment_node_ids(file_path)
            num_nodes, num_edges = get_max_node_and_edge_count(file_path)
            add_header_to_file(file_path, num_nodes, num_edges)
            print(f"Processing file: {file_path}")


folder_path = './test/'
process_folder(folder_path)
