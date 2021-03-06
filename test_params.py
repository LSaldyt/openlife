#!/usr/bin/env python3
import subprocess, shutil, sys, os
import signal
import pickle

import matplotlib.pyplot as plt

from collections import namedtuple, OrderedDict
from contextlib  import contextmanager
from pprint      import pprint
from copy        import deepcopy
from statistics  import stdev
from math        import sqrt

import plot

def timeout_run(command, seconds=5):
    with subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, preexec_fn=os.setsid) as process:
        try:
            output = process.communicate(timeout=seconds)[0]
        except subprocess.TimeoutExpired:
            print('Timeout occured')
            os.killpg(process.pid, signal.SIGINT) # send signal to the process group
            output = process.communicate()[0]
        return output

Variance = namedtuple('Variance', ['name', 'begin', 'end', 'step'])

def to_variance(line):
    name, begin, end, step = line.split()
    try:
        return Variance(name, int(begin), int(end), int(step))
    except ValueError:
        return Variance(name, float(begin), float(end), float(step))

def read_variances_file(filename):
    with open(filename, 'r') as variancefile:
        return [to_variance(line) for line in variancefile]

def variance_set(variance):
    varSet = []
    n = variance.begin
    while n <= variance.end:
        varSet.append((variance.name, n))
        n += variance.step
    return varSet

def generate_variances(filename):
    variances = read_variances_file(filename)
    return (variance_set(variance) for variance in variances)

def read_param_file(filename):
    paramdict = OrderedDict()
    with open(filename, 'r') as params:
        for line in params:
            if len(line.split()) == 2:
                name, value = line.split()
                paramdict[name] = value
    return paramdict

def write_params(params, filename):
    with open(filename, 'w') as outfile:
        for param in params.items():
            outfile.write('%s %s\n' % param)

def get_metrics(iterations=1, max_time=5):
    metrics = OrderedDict()
    for key in plot.view_keys:
        metrics[key] = [] 
    for _ in range(iterations):
        timeout_run('./openlifecli __temp_params__.txt', max_time)
        for key in plot.view_keys:
            metrics[key].append(plot.get_last_value(key)) 
        print('.', end='', flush=True)
    print('')
    return metrics

def clean_output(i):
    datadir = 'output/param_testing/data_%s' % i
    if os.path.exists(datadir):
        shutil.rmtree(datadir)
    shutil.copytree('output/data', datadir)
    filename = 'output/param_testing/params_%s' % i
    if os.path.isfile(filename):
        os.remove(filename)
    shutil.copy('__temp_params__.txt', filename)

def clean_data_dir():
    if os.path.exists('output/data'):
        shutil.rmtree('output/data')
        os.makedirs('output/data')

def print_kv(key, value):
    print('%-18s| %s' % (key, value))

def print_metrics(metrics, name, value):
    print_kv(name, value)
    for k, v in metrics.items():
        print_kv(k, v)
    print('-' * 80)

def average(data):
    return sum(data) / len(data)

def t_val_test(x, paramVals, mean):
    signed_t = (average(paramVals) -  mean)/(max(0.000001, stdev(paramVals)) / sqrt(len(paramVals)))
    sign = '+' if signed_t > 0 else '-'
    t = abs(signed_t)
    result = 'insignificant'
    if t > .883:  result = '>80%'
    if t > 1.1:   result = '>85%'
    if t > 1.383: result = '>90%'
    if t > 1.833: result = '>95%'
    if t > 2.821: result = '>99%'
    return '%-7s, %-14s %s (t=%s)\n' % (x, sign, result, signed_t)

def test_hypothesis(key, plotItems, x, filename):
    hypothesis_string = ''
    assert len(plotItems) > 1
    startMean = average(plotItems[0])
    lastMean = startMean 
    for i, paramVals in enumerate(plotItems[1:]):
        hypothesis_string += t_val_test(x[i], paramVals, lastMean)
        lastMean = average(paramVals)
    hypothesis_string += 'mean test from start sample\n'
    for i, paramVals in enumerate(plotItems[1:]):
        hypothesis_string += t_val_test(x[i], paramVals, startMean)
    with open(filename, 'w') as hypothesis_file:
       hypothesis_file.write('%s\n%s' % (key, hypothesis_string)) 

def plot_metrics(metricDicts, metricKeys):
    pprint(metricDicts)
    for metricName, valueDict in metricDicts.items():
        x = list(valueDict.keys())
        fig, axarr = plt.subplots(len(metricKeys), sharex=True)
        plt.tight_layout(pad=1.0, h_pad=1.0)
        fig.subplots_adjust(top=0.88, bottom=.12, left=.12)
        for i, key in enumerate(metricKeys):
            if key != 'location':
                plotItems = [v[key] for (k, v) in valueDict.items()]
                print(key)
                test_hypothesis(key, plotItems, x, 'output/param_testing/%s_%s_hypothesis' % (metricName, key))
                axarr[i].boxplot(plotItems, labels=x)
                axarr[i].set_title(key)
        plt.suptitle('Metrics when varying %s' % metricName, size=16) # plt.title() would set the last-added subplot, which is less than ideal ;)
        fig.text(0.5, 0.04, metricName, ha='center')
        plt.savefig('output/images/%s.png' % metricName)
        plt.show()

def main(useSaved=False):
    clean_data_dir()

    try:
        if not useSaved:
            subprocess.call('./build.sh', shell=True)
            defaultParams = read_param_file('params.txt')
            varianceSets  = generate_variances('variances.txt')
            params        = deepcopy(defaultParams) 

            metricDicts = OrderedDict()
            for i, varSet in enumerate(varianceSets):
                for name, value in varSet:
                    params[name] = value
                    write_params(params, '__temp_params__.txt') 
                    metrics = get_metrics(iterations=10, max_time=60)
                    print_metrics(metrics, name, value)
                    if name not in metricDicts:
                        metricDicts[name] = OrderedDict()
                    metricDicts[name][value] = metrics
                    clean_output(i)
                    clean_data_dir()

                params = deepcopy(defaultParams)
            with open('output/param_testing/metricDicts.pkl', 'wb') as pickleFile:
                pickle.dump(metricDicts, pickleFile)
        else:
            with open('output/param_testing/metricDicts.pkl', 'rb') as pickleFile:
                metricDicts = pickle.load(pickleFile)
        plot_metrics(metricDicts, ['complexity', 'network_count', 'network_size', 'entropy', 'area', 'population', 'density'])
    except KeyboardInterrupt:
        plot_metrics(metricDicts, ['complexity', 'network_count', 'network_size', 'entropy', 'area', 'population', 'density'])

    if os.path.isfile('__temp_params__.txt'):
        os.remove('__temp_params__.txt')

if __name__ == '__main__':
    useSaved = len(sys.argv) > 1
    main(useSaved)
