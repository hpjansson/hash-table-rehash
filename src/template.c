#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#define STR_MAX 256

double get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

static void
gen_string_from_integer(char *str_out, int num)
{
    snprintf(str_out, STR_MAX, "mystring-%d", num);
}

int main(int argc, char ** argv)
{
    int num_keys = atoi(argv[1]);
    int i, value = 0xcafecafe;
    char str [STR_MAX];

    if(argc <= 2)
        return 1;

    SETUP

    double before = get_time();

    if(!strcmp(argv[2], "sequential"))
    {
        for(i = 0; i < num_keys; i++)
            INSERT_INT_INTO_HASH(i, value);
    }

    else if(!strcmp(argv[2], "spaced"))
    {
        for(i = 0; i < num_keys; i++)
            INSERT_INT_INTO_HASH(i * 256, value);
    }

    else if(!strcmp(argv[2], "random"))
    {
        srandom(1); // for a fair/deterministic comparison
        for(i = 0; i < num_keys; i++)
            INSERT_INT_INTO_HASH((int)random() & 0x0fffffff, value);
    }

    else if(!strcmp(argv[2], "delete"))
    {
        for(i = 0; i < num_keys; i++)
            INSERT_INT_INTO_HASH(i, value);
        before = get_time();
        for(i = 0; i < num_keys; i++)
            DELETE_INT_FROM_HASH(i);

        /* Required to make some implementations release memory */
        INSERT_INT_INTO_HASH(1, value);
    }

    else if(!strcmp(argv[2], "aging"))
    {
        srandom(1); // for a fair/deterministic comparison

        /* First populate the table */
        for(i = 0; i < num_keys; i++)
            INSERT_INT_INTO_HASH((int)random() % num_keys, value);

        before = get_time();

        /* Randomly insert and delete keys for a bit */
        for(i = 0; i < num_keys; i++)
        {
            DELETE_INT_FROM_HASH((int)random() % num_keys);
            INSERT_INT_INTO_HASH((int)random() % num_keys, value);
        }
    }

    else if(!strcmp(argv[2], "sequentialstring"))
    {
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, i);
            INSERT_STR_INTO_HASH (str, value);
        }
    }

    else if(!strcmp(argv[2], "randomstring"))
    {
        srandom(1); // for a fair/deterministic comparison
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, random ());
            INSERT_STR_INTO_HASH (str, value);
        }
    }

    else if(!strcmp(argv[2], "deletestring"))
    {
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, i);
            INSERT_STR_INTO_HASH (str, value);
        }

        before = get_time();
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, i);
            DELETE_STR_FROM_HASH (str);
        }
    }

    else if(!strcmp(argv[2], "agingstring"))
    {
        srandom(1); // for a fair/deterministic comparison

        /* First populate the table */
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, random () % num_keys);
            INSERT_STR_INTO_HASH (str, value);
        }

        before = get_time();

        /* Randomly insert and delete keys for a bit */
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, random() % num_keys);
            DELETE_STR_FROM_HASH (str);

            gen_string_from_integer (str, random() % num_keys);
            INSERT_STR_INTO_HASH (str, value);
        }
    }

    else if(!strcmp(argv[2], "small"))
    {
        int keys_per_table = num_keys / 10000;
        int j;

        srandom(1); // for a fair/deterministic comparison

        for (j = 0; j < 10000; j++)
        {
            /* First populate the table */
            for(i = 0; i < keys_per_table; i++)
                INSERT_INT_INTO_HASH((int)random() % keys_per_table, value);

            /* Randomly insert and delete keys for a bit */
            for(i = 0; i < keys_per_table * 4; i++)
            {
                DELETE_INT_FROM_HASH((int)random() % keys_per_table);
                INSERT_INT_INTO_HASH((int)random() % keys_per_table, value);
            }

            /* Delete all keys */
            for(i = 0; i < keys_per_table; i++)
            {
                DELETE_INT_FROM_HASH(i);
            }
        }
    }

    double after = get_time();
    printf("%f\n", after-before);
    fflush(stdout);
    sleep(1000000);
}
