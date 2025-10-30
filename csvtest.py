import csv
import sys

def read_csv(filename):
    """
    Read a CSV file and return a list of dictionaries.
    Each row contains two columns: 'number' and 'multiple_of'.
    """
    try:
        rows = []
        with open(filename, newline='', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    number = int(row['number'])
                    multiple = int(row['multiple_of'])
                except (KeyError, ValueError):
                    raise ValueError(f"Invalid row: {row}")
                rows.append({'number': number, 'multiple_of': multiple})
        return rows
    except FileNotFoundError:
        print(f"File not found: {filename}")
        sys.exit(1)

def check_multiples(rows):
    """
    Check whether each number is a multiple of multiple_of.
    Return a list: [(number, multiple, status), ...]
    """
    results = []
    for row in rows:
        number = row['number']
        multiple = row['multiple_of']
        if multiple == 0:
            status = "Not a multiple"   # Prevent division by zero
        else:
            status = "Is a multiple" if number % multiple == 0 else "Not a multiple"
        results.append((number, multiple, status))
    return results

def display_results(results):
    """
    Print results in table format.
    """
    print(f"{'Number':<10}{'Multiple Of':<15}{'Result':<20}")
    print("-" * 45)
    for number, multiple, status in results:
        print(f"{number:<10}{multiple:<15}{status:<20}")

def run_tests():
    """
    Simple unit test to verify the logic of check_multiples.
    """
    test_data = [
        {'number': 10, 'multiple_of': 2},
        {'number': 15, 'multiple_of': 5},
        {'number': 21, 'multiple_of': 4},
    ]
    expected = ["Is a multiple", "Is a multiple", "Not a multiple"]
    actual = [r[2] for r in check_multiples(test_data)]
    assert actual == expected, f"Test failed: {actual} != {expected}"
    print("All tests passed")

def main():
    """
    Main process: read CSV, compute, and display results.
    """
    filename = "multiples.csv"
    rows = read_csv(filename)
    results = check_multiples(rows)
    display_results(results)

if __name__ == "__main__":
    # Enable tests when needed
    run_tests()
    main()