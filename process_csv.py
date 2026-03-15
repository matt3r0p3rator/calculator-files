import csv

input_path = '/home/matt3r/calculator-files/periodic_table_data/oxidation_states.csv'
final_path = '/home/matt3r/calculator-files/periodic_table_data/periodictable_final.csv'

# Read oxidation states and clean it
ox_data = {}
with open(input_path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

new_ox_lines = []
for line in lines:
    parts = line.strip().split('\t')
    if len(parts) >= 5:
        atomic_num = parts[0].strip()
        name = parts[1].strip()
        symbol = parts[2].strip()
        pos = parts[3].strip()
        neg = parts[4].strip()
        new_ox_lines.append([atomic_num, name, symbol, pos, neg])
        ox_data[atomic_num] = {'pos': pos, 'neg': neg}

# Write back oxidation states as CSV
with open(input_path, 'w', encoding='utf-8', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['AtomicNumber', 'ElementName', 'Symbol', 'PositiveOxidationStates', 'NegativeOxidationStates'])
    writer.writerows(new_ox_lines)

# Read final table
with open(final_path, 'r', encoding='utf-8') as f:
    reader = csv.reader(f)
    final_rows = list(reader)

header = final_rows[0]
if 'PositiveOxidationStates' not in header:
    header.extend(['PositiveOxidationStates', 'NegativeOxidationStates'])

for row in final_rows[1:]:
    num = str(row[0]).strip()
    if num in ox_data:
        if len(row) < len(header):
            row.extend([ox_data[num]['pos'], ox_data[num]['neg']])
    else:
        if len(row) < len(header):
            row.extend(['', ''])

with open(final_path, 'w', encoding='utf-8', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(final_rows)

print("Done")
