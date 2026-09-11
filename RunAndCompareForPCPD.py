#!/usr/bin/env python

import subprocess
import json
import time
import argparse
import sys
from enum import Enum
from pathlib import Path
import pandas as pd

SMALL_EPS=1E-14

class Action(str,Enum):
    """
    Enumeration of supported testing actions.
    Inherits from str to allow direct string comparisons.
    """
    PRICE = 'Price'
    PORTFOLIO = 'Portfolio'
    PNL = 'Pnl'

    def lower(self):
        return self.value.lower()

PRICE_EXEC_REGEXP = '**/price0*'
PORTFOLIO_EXEC_REGEXP = '**/hedge*'
PNL_EXEC_REGEXP = '**/pnl*'

PNL_OR_PORTFOLIO_TIMEOUT = 500
PRICE_TIMEOUT = 60
action = None
missing_parameters = False
compare_only = False
run_executables = True
dashboard_only = False
outdir = None
datadir = None
execdir = None
execpath = None
marketdir = None
exec_folder = None
market_folder = None

parser = argparse.ArgumentParser()
parser.add_argument("--toplevel", help="Top level directory for the whole structure containing the following directories 'Executables' and 'Tests'. This option takes precedence over all others.", type=str)
parser.add_argument("--exec", help="Path to executable. Optional if --toplevel is specified.", type=str)
parser.add_argument("--datadir", help="Path to input data directory. Optional if --toplevel is specified.", type=str)
parser.add_argument("--marketdir", help="Where to look for market data. To be used only with --hedge option. If not specified, use the same directory as for --datadir", type=str)
parser.add_argument("--outdir", help="Where to write the output. Optional if --toplevel is specified.", type=str)
parser.add_argument("--compare-only", help="Only compare the results. Do not run executables.", action="store_true")
parser.add_argument("--dashboard-only", help="Generate a dashboard of the results. Do not run executables.", action="store_true")
group = parser.add_mutually_exclusive_group()
group.add_argument("--price", help="Test the prices at time 0.", action="store_true")
group.add_argument("--pnl", help="Test the P&L.", action="store_true")
group.add_argument("--portfolio", help="Test the whole portfolio.", action="store_true")

args = parser.parse_args()

# --- Argument Parsing and Path Resolution ---
if args.toplevel:
    structuredir = args.toplevel
    outdir = Path(structuredir).resolve()
    exec_folder = outdir / "Executables"
    datadir = outdir / "Tests"
    marketdir = datadir
else:
    if not args.exec:
        print('Missing "--exec" parameter')
        missing_parameters = True
    else:
        execpath = Path(args.exec).resolve()
    if not args.datadir:
        print('Missing "--datadir" parameter')
        missing_parameters = True
    else:
        datadir = Path(args.datadir)
    if not args.outdir:
        print('Missing "--outdir" parameter')
        missing_parameters = True
    else:
        outdir = Path(args.outdir)
if args.price:
    action = Action.PRICE
elif args.pnl or args.portfolio:
    if args.pnl:
        action = Action.PNL
    if args.portfolio:
        action = Action.PORTFOLIO
    if marketdir is None and args.marketdir:
        marketdir = Path(args.marketdir)
    else:
        marketdir = datadir
else:
    print('Missing action parameter: "--price", "--pnl" or "--portfolio"')
    missing_parameters = True
if args.compare_only:
    compare_only = True
    run_executables = False
if args.dashboard_only:
    dashboard_only = True
    run_executables = False
    if execpath:
        print('Cannot generate dashboard if --exec is specified')
        missing_parameters = True
if missing_parameters:
    sys.exit(1)


# Top level Structure to write computation results and comparison outputs
outdir_folder = outdir.resolve()
outdir_folder.mkdir(parents=True, exist_ok=True)
# Directory containing all the tests.
test_folder = datadir.resolve()
if marketdir:
    market_folder = marketdir.resolve()

def get_obtained_suffix(action:str):
    return '_obtained_' + action

def get_expected_suffix(action:str):
    return '_expected_' + action

def get_result_suffix(action:str):
    return '_result_' + action


def get_all_test_cases(test_folder: Path):
    """Returns a list of all .json files in the test folder, excluding 'expected' files."""

    return [f for f in test_folder.iterdir() if f.suffix == ".json" and not f.match('*_expected*.json')]


def compare_prices0(expected: dict, obtained: dict, name='price'):
    """
    Compare the prices at time 0 using the std deviation of the MC estimator
    """
    expected_price = expected[name]
    expected_std_dev = expected[name + "StdDev"]
    obtained_price = obtained[name]
    obtained_std_dev = obtained[name + "StdDev"]
    if not (isinstance(obtained_price, float) and isinstance(obtained_std_dev, float)):
        same_interval = None
        std_dev_ratio = None
    else:
        same_interval = abs(expected_price - obtained_price) / (expected_std_dev + SMALL_EPS)
        std_dev_ratio = (obtained_std_dev  + SMALL_EPS) / (expected_std_dev + SMALL_EPS)
    return (same_interval, std_dev_ratio)


def compare_deltas(expected: dict, obtained: dict, name='delta'):
    """
    Compare the deltas at time 0 using the std deviation of the MC estimator
    Returns: (Max normalized distance across all components, Max std dev ratio).

    """
    expected_delta = expected[name]
    obtained_delta = obtained[name]
    expected_delta_std_dev = expected[name + "StdDev"]
    obtained_delta_std_dev = obtained[name + "StdDev"]
    delta_same_interval = []
    delta_std_dev_ratio = []
    for ind, val in enumerate(expected_delta):
        if not (isinstance(obtained_delta[ind], float) and isinstance(obtained_delta_std_dev[ind], float)):
            return None, None
        delta_same_interval.append(abs(val - obtained_delta[ind]) / (expected_delta_std_dev[ind] + SMALL_EPS))
        delta_std_dev_ratio.append((obtained_delta_std_dev[ind] + SMALL_EPS) / (expected_delta_std_dev[ind] + SMALL_EPS))
    return max(delta_same_interval), max(delta_std_dev_ratio)


def compare_pnl(expected: dict, obtained: dict):
    """Returns the absolute difference between obtained and expected finalPnL."""
    obtained_pnl = obtained["finalPnL"]
    expected_pnl = expected["finalPnL"]
    if not isinstance(obtained_pnl, float):
        pnl_difference = None
    else:
        pnl_difference = abs(obtained_pnl - expected_pnl)
    return pnl_difference


def compare_times(expected: dict, obtained: dict):
    """Returns the ratio of obtained execution time vs expected execution time."""
    obtained_time = obtained["time"]
    expected_time = expected["time"]
    time_difference = obtained_time / expected_time
    return time_difference

def compare_portfolios(expected:dict, obtained: dict):
    """
    Aggregates comparisons for all items in a portfolio.
    Returns: (Max Value Dist, Max Price Dist, Price StdDev Ratio, Max Delta Dist, Delta StdDev Ratio).
    """
    obtained_portfolio = obtained["portfolio"]
    expected_portfolio = expected["portfolio"]
    max_valueDistance = 0.
    max_priceDistance = 0.
    max_priceStdDevRatio = 0.
    min_priceStdDevRatio = 1E10
    max_deltaComponentDistance = 0.
    max_deltaStdDevRatios = 0.
    min_deltaStdDevRatios = 1E10
    for obtained_v, expected_v in zip(obtained_portfolio, expected_portfolio):
        max_valueDistance = max(max_valueDistance, abs(obtained_v["value"] - expected_v["value"]))
        price_same_interval, price_std_dev_ratio = compare_prices0(expected_v, obtained_v)
        max_priceDistance = max(max_priceDistance, price_same_interval)
        max_priceStdDevRatio  = max(max_priceStdDevRatio, price_std_dev_ratio)
        min_priceStdDevRatio = min(min_priceStdDevRatio, price_std_dev_ratio)
        delta_same_interval, delta_std_dev_ratio = compare_deltas(expected_v, obtained_v, "deltas")
        max_deltaComponentDistance = max(max_deltaComponentDistance, delta_same_interval)
        max_deltaStdDevRatios  = max(max_deltaStdDevRatios, delta_std_dev_ratio)
        min_deltaStdDevRatios = min(min_deltaStdDevRatios, delta_std_dev_ratio)

    return max_valueDistance, max_priceDistance, max(max_priceStdDevRatio, 1. / min_priceStdDevRatio), max_deltaComponentDistance, max(max_deltaStdDevRatios, 1. / min_deltaStdDevRatios)


def compare_pricing_results(output_path: Path, expected_path: Path):
    """ Compare the time-0 results """
    with output_path.open() as output_stream:
        with expected_path.open() as expected_stream:
            expected = json.load(expected_stream)
            obtained = json.load(output_stream)
            # is obtained price in the same interval as expected price?
            same_interval, std_dev_ratio = compare_prices0(expected, obtained)
            delta_same_interval, delta_std_dev_ratio = compare_deltas(expected, obtained)
            # computational time
            time_ratio = compare_times(expected, obtained)
            result = {
                "priceDistance":same_interval,
                "stdDevRatio": std_dev_ratio,
                "deltaComponentDistance": delta_same_interval,
                "deltaStdDevRatios": delta_std_dev_ratio,
                "timeRatio": time_ratio
            }
            return result

def compare_pnl_results(output_path: Path, expected_path: Path):
    """ Compare the pnl results """
    with output_path.open() as output_stream:
        with expected_path.open() as expected_stream:
            expected = json.load(expected_stream)
            obtained = json.load(output_stream)
            # Is the obtained price in the same interval as expected price?
            same_interval, std_dev_ratio = compare_prices0(expected, obtained, 'initialPrice')
            pnl_difference = compare_pnl(expected, obtained)
            # computational time
            time_ratio = compare_times(expected, obtained)
            result = {
                "priceDistance":same_interval,
                "stdDevRatio": std_dev_ratio,
                "pnlDifference": pnl_difference,
                "timeRatio": time_ratio
            }
            return result


def compare_portfolio_results(output_path: Path, expected_path: Path):
    """ Compare the portfolio results """
    with output_path.open() as output_stream:
        with expected_path.open() as expected_stream:
            expected = json.load(expected_stream)
            obtained = json.load(output_stream)
            # computational time
            time_ratio = compare_times(expected, obtained)
            # Find the maximum difference in portfolio entries
            valueMaxDistance, priceMaxDistance, priceStdDevMaxRatio, deltaMaxDistance, deltaStdDevMaxRatio = compare_portfolios(expected, obtained)
            result = {
                "valueMaxDistance": valueMaxDistance,
                "priceMaxDistance": priceMaxDistance,
                "priceStdDevMaxRatio": priceStdDevMaxRatio,
                "deltaMaxDistance": deltaMaxDistance,
                "deltaStdDevMaxRatio": deltaStdDevMaxRatio,
                "timeRatio": time_ratio
            }
            return result


def run_allexec_tests(action):
    for exe in exec_folder.iterdir():
        # if not exe.match('**/exec_*'):
        #     continue
        if action == Action.PRICE and not exe.match(PRICE_EXEC_REGEXP):
            continue
        if action == Action.PORTFOLIO and not exe.match(PORTFOLIO_EXEC_REGEXP):
            continue
        if action == Action.PNL and not exe.match(PNL_EXEC_REGEXP):
            continue
        exe_fullpath = exec_folder / exe
        run_singleexec_test(action, exe_fullpath)


def run_singleexec_test(action, exe_fullpath):
    """
    Runs a single binary against all test cases in the data directory.
    Outputs results to: {outdir}/Outputs/{exe}_output/{test_folder_name}/{test_case}_obtained_{action}.json
    """
    output_folder = outdir_folder / "Outputs" / (exe_fullpath.stem + "_output")
    output_folder.mkdir(parents=True, exist_ok=True)
    scenario_folder = output_folder / test_folder.name
    scenario_folder.mkdir(parents=True, exist_ok=True)
    for test_case in get_all_test_cases(test_folder):
        print('Running on test case', test_case)
        outfile_path = None
        timeout = None
        proc_args = [exe_fullpath.as_posix()]
        outfile_path = scenario_folder / (test_case.stem + get_obtained_suffix(action.lower()) + ".json")
        if action == Action.PRICE:
            proc_args.append(test_case.as_posix())
            timeout = PRICE_TIMEOUT
        elif action in (Action.PORTFOLIO, Action.PNL):
            test_case_market = market_folder / (test_case.stem + '_market.txt')
            if not test_case_market.exists:
                print('Market file ', test_case_market.as_posix(), ' does not exist')
                continue
            proc_args.extend([test_case_market.as_posix(), test_case.as_posix()])
            timeout = PNL_OR_PORTFOLIO_TIMEOUT
        else:
            print("Unknown action")
            return

        print("Running ", proc_args)
        try:
            start_time = time.time()
            exec_output = subprocess.run(proc_args, stdout=subprocess.PIPE, timeout=timeout, check=True)
            end_time = time.time()
            exec_result = json.loads(exec_output.stdout.decode('utf-8'))
            dict_result = {
                "time": end_time - start_time,
            }
            if not isinstance(exec_result, dict):
                raise json.JSONDecodeError("Output is not of JSON type", exec_output.stdout.decode('utf-8'), 0)
            dict_result.update(exec_result)
            with outfile_path.open(mode='w') as outfile:
                json.dump(dict_result, outfile, indent=4)
        except json.JSONDecodeError:
            print('Output of "', ' '.join(proc_args), '" is not of JSON type')
            print(exec_output.stdout)
        except subprocess.CalledProcessError as e:
            print('--', e)
            print('--', e.output.decode('utf-8'))
        except subprocess.TimeoutExpired:
            print('Timeout running ', proc_args)

def run_compare_results(action, exe_fullpath=None):
    """
    Matches 'obtained' JSON files with 'expected' JSON files and computes comparison metrics.
    Writes results to: {outdir}/Results/{exe}_result_{action}.json
    """
    for output_folder in (outdir_folder / "Outputs").iterdir():
        if sum(token[0] == '.' for token in output_folder.relative_to(outdir_folder).parts):
            continue
        if (exe_fullpath is not None and not output_folder.match('**/' + exe_fullpath.stem + '_output')) or (exe_fullpath is None and not output_folder.match('**/*_output')):
            continue
        exe = None
        expected_suffix = None
        obtained_suffix = None
        result_suffix = None
        compare_function = None
        obtained_suffix = get_obtained_suffix(action.lower())
        expected_suffix = get_expected_suffix(action.lower())
        result_suffix = get_result_suffix(action.lower())
        if action == Action.PRICE:
            compare_function = compare_pricing_results
            if not output_folder.match(PRICE_EXEC_REGEXP):
                continue
        elif action == Action.PORTFOLIO:
            compare_function = compare_portfolio_results
            if not output_folder.match(PORTFOLIO_EXEC_REGEXP):
                continue
        elif action == Action.PNL:
            compare_function = compare_pnl_results
            if not output_folder.match(PNL_EXEC_REGEXP):
                continue
        else:
            print("Unknown action")
            return
        exe = output_folder.stem.replace(obtained_suffix, '')
        result_folder = outdir_folder / "Results"
        result_folder.mkdir(parents=True, exist_ok=True)
        test_case_results = []
        for test_case in get_all_test_cases(test_folder):
            print('Comparing results on test case', test_case)
            expected_file = test_case.parent / (test_case.stem + expected_suffix + '.json')
            output_file = output_folder / datadir.stem / (test_case.stem + obtained_suffix + '.json')
            if not output_file.exists():
                print('File ', output_file, ' does not exist')
                continue
            print(exe + ": comparing " + expected_file.name + " with " + output_file.name)
            comparison = compare_function(output_file, expected_file)
            test_case_comparison = {
                "testCaseName": test_case.stem,
                "comparison": comparison
                }
            test_case_results.append(test_case_comparison)
        result_file = result_folder / (exe + result_suffix + '.json')
        with result_file.open('w') as result_stream:
            json.dump(test_case_results, result_stream, indent=4)

def generate_dashboard(action):
    """
    Reads all result JSONs in the Results folder and pivots them into a
    single CSV dashboard using Pandas.
    """
    result_folder = outdir_folder / "Results"
    if not result_folder.exists():
        print('Make sure to run the comparison first')
        return

    result_suffix = get_result_suffix(action.lower())
    results = {}
    for f in result_folder.iterdir():
        if not f.match('*' + result_suffix + '.json'):
            continue
        name = f.stem.replace(result_suffix, '')
        with f.open() as result_stream:
            results[name] = json.load(result_stream)
    # allTestCases = set(t.get('testCaseName') for r in results.values() for t in r)
    dashboardDf = pd.DataFrame()
    for team, teamResults in results.items():
        if len(teamResults) == 0:
            continue
        teamDf = pd.DataFrame()
        for case in teamResults:
            df = pd.DataFrame.from_dict(case['comparison'], orient='index')
            df = df.add_prefix(case['testCaseName'] + '_', axis='index')
            teamDf = pd.concat([teamDf, df])
        teamDf.columns = [team]
        dashboardDf = dashboardDf.merge(teamDf, how='outer', left_index=True, right_index=True)
    dashboardDf = dashboardDf.reindex(sorted(dashboardDf.columns), axis=1)
    filename = result_folder / ('dashboard_' + action.lower() + '.csv')
    print('Writing dashboard to ', filename)
    dashboardDf.to_csv(filename)


if __name__ == "__main__":
    if dashboard_only:
        generate_dashboard(action)
        sys.exit(0)
    if execpath:
        if run_executables:
            run_singleexec_test(action, execpath)
        run_compare_results(action, execpath)
    else:
        if run_executables:
            run_allexec_tests(action)
    run_compare_results(action)
    generate_dashboard(action)
