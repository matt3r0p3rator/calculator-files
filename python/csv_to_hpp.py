import csv
import sys

def get_str(row, key):
    # Escape quotes and handle empty strings
    val = row.get(key, '').strip().replace('"', '\\"')
    
    # Replace unicode superscript numbers with regular numbers 
    # since Ndless/nSDL fonts typically only support basic ASCII 0-127
    superscripts = {
        '⁰': '0', '¹': '1', '²': '2', '³': '3', '⁴': '4',
        '⁵': '5', '⁶': '6', '⁷': '7', '⁸': '8', '⁹': '9'
    }
    for sup, reg in superscripts.items():
        val = val.replace(sup, '^' + reg)
        
    return f'"{val}"'

def get_int(row, key):
    val = row.get(key, '').strip()
    return val if val else "0"

def get_float(row, key):
    val = row.get(key, '').strip()
    return val if val else "0.0"

def generate_hpp(csv_path, output_path):
    import os
    delimiter = '\t' if csv_path.endswith('.tsv') else ','
    with open(csv_path, 'r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f, delimiter=delimiter)
        
        lines = []
        lines.append('#ifndef CHEMISTRY_DATA_HPP')
        lines.append('#define CHEMISTRY_DATA_HPP')
        lines.append('')
        lines.append('#include "chemistry.hpp"')
        lines.append('#include <vector>')
        lines.append('')
        lines.append('const std::vector<Element> PERIODIC_TABLE_DATA = {')
        
        for row in reader:
            # Map CSV columns to C++ struct fields.
            atomic_number = get_int(row, 'AtomicNumber')
            symbol = get_str(row, 'Symbol')
            name = get_str(row, 'ElementName')
            atomic_weight = get_float(row, 'AtomicMass')
            category = get_str(row, 'Category')
            group = get_str(row, 'Group')
            period = get_str(row, 'Period')
            block = get_str(row, 'Block')
            electron_configuration = get_str(row, 'ElectronConfiguration')
            electronegativity = get_float(row, 'Electronegativity')
            density = get_str(row, 'Density')
            number_of_neutrons = get_int(row, 'NumberofNeutrons')
            number_of_protons = get_int(row, 'NumberofProtons')
            number_of_electrons = get_int(row, 'NumberofElectrons')
            atomic_radius = get_float(row, 'AtomicRadius')
            first_ionization = get_float(row, 'FirstIonization')
            melting_point = get_float(row, 'MeltingPoint')
            boiling_point = get_float(row, 'BoilingPoint')
            number_of_isotopes = get_int(row, 'NumberOfIsotopes')
            discoverer = get_str(row, 'Discoverer')
            year = get_str(row, 'Year')
            specific_heat = get_float(row, 'SpecificHeat')
            number_of_shells = get_int(row, 'NumberofShells')
            number_of_valence = get_int(row, 'NumberofValence')
            positive_oxidation_states = get_str(row, 'PositiveOxidationStates')
            negative_oxidation_states = get_str(row, 'NegativeOxidationStates')

            element_str = f"    {{{atomic_number}, {symbol}, {name}, {atomic_weight}, {category}, {group}, {period}, {block}, {electron_configuration}, {electronegativity}, {density}, {number_of_neutrons}, {number_of_protons}, {number_of_electrons}, {atomic_radius}, {first_ionization}, {melting_point}, {boiling_point}, {number_of_isotopes}, {discoverer}, {year}, {specific_heat}, {number_of_shells}, {number_of_valence}, {positive_oxidation_states}, {negative_oxidation_states}}},"
            lines.append(element_str)
        
        lines.append('};')
        lines.append('')
        lines.append('#endif // CHEMISTRY_DATA_HPP')

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    
    print(f"Successfully generated {output_path} from {csv_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python csv_to_hpp.py <input.csv> <output.hpp>")
        sys.exit(1)
    
    generate_hpp(sys.argv[1], sys.argv[2])