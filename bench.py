import sys, os, subprocess, signal

programs = [
    'glib_hash_table',
    'stl_unordered_map',
    'boost_unordered_map',
    'google_sparse_hash_map',
    'google_dense_hash_map',
    'qt_qhash',
    'python_dict',
    'ruby_hash',
    'khash'
]

minkeys  =       100 * 1000
maxkeys  =  5 * 1000 * 1000
interval =  1 * 1000 * 1000
best_out_of = 2

# for the final run, use this:
#minkeys  =  2*1000*1000
#maxkeys  = 40*1000*1000
#interval =  2*1000*1000
#best_out_of = 3
# and use nice/ionice
# and shut down to the console
# and swapoff any swap files/partitions

outfile = open('output', 'w')

if len(sys.argv) > 1:
    benchtypes = sys.argv[1:]
else:
    benchtypes = ('sequential', 'spaced', 'random', 'delete', 'aging')

for benchtype in benchtypes:
    nkeys = minkeys
    while nkeys <= maxkeys:
        for program in programs:
            fastest_attempt = 1000000
            fastest_attempt_data = ''

            # Spaced integers break Google sparsehash. Cap its iterations so
            # it doesn't take forever.
            if nkeys >= 5000000 and 'google_' in program and benchtype is 'spaced':
                continue

            for attempt in range(best_out_of):
                proc = subprocess.Popen(['./build/'+program, str(nkeys), benchtype], stdout=subprocess.PIPE)

                # wait for the program to fill up memory and spit out its "ready" message
                try:
                    runtime = float(proc.stdout.readline().strip())
                except:
                    runtime = 0

                # pmap output can look like this:
                #
                # 3646:   bash
                # 0000564b4a271000     964       0       0       0       0 r-xp- /bin/bash
                # ...
                #
                # We use tail to skip the first line. The columns we're looking for are
                # 'Dirty' plus 'Swap' -- columns 4 and 5, counting from 0.

                ps_proc = subprocess.Popen(['pmap -q %d | tail -n+2' % proc.pid], shell=True, stdout=subprocess.PIPE)
                nbytes = 0
                while True:
                    line = ps_proc.stdout.readline()
#                    print line
                    try:
                        nbytes += (int(line.split()[4]) + int(line.split()[5])) * 1024
                    except:
                        break

                ps_proc.wait()

                os.kill(proc.pid, signal.SIGKILL)
                proc.wait()

                if nbytes and runtime: # otherwise it crashed
                    line = ','.join(map(str, [benchtype, nkeys, program, nbytes, "%0.6f" % runtime]))

                    if runtime < fastest_attempt:
                        fastest_attempt = runtime
                        fastest_attempt_data = line

            if fastest_attempt != 1000000:
                print >> outfile, fastest_attempt_data
                print fastest_attempt_data

        nkeys += interval
