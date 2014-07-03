/*
This file is part of mfaktc (mfakto).
Copyright (C) 2014  Oliver Weihe (o.weihe@t-online.de)
                    Bertram Franz (bertramf@gmx.net)

mfaktc (mfakto) is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

mfaktc (mfakto) is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with mfaktc (mfakto).  If not, see <http://www.gnu.org/licenses/>.
*/

/* code to handle dynamic changing of settings */

#if defined(_MSC_VER) || defined(__MINGW32__)
  #include <conio.h>
#else
  #include "kbhit.h"
  static keyboard kb;
  #define _kbhit kb.kbhit
  #define _getch kb.getch
#endif

#include "compatibility.h"
#include "my_types.h"
#include <stdio.h>
#include <iostream>


static void print_menu(mystuff_t *mystuff)
{
  puts("\nSettings menu\n\nNum  Setting     Current value  (shortcut outside of the menu for de-/increasing this setting)\n");

  printf("  1  SievePrimes       = %-8u  (-/+)\n", mystuff->gpu_sieving ? mystuff->gpu_sieve_primes : mystuff->sieve_primes);
  printf("  2  SieveSize         = %-8u  (s/S)\n", mystuff->gpu_sieving ? mystuff->gpu_sieve_size/1024/1024 : mystuff->sieve_size/8192);
  printf("  3  SieveProcessSize  = %-8u  (p/P)\n", mystuff->gpu_sieving ? mystuff->gpu_sieve_processing_size/1024 : mystuff->sieve_size/8192);
  printf("  4  SievePrimesAdjust = %-8u  (a/A)\n", mystuff->gpu_sieving ? 0 : mystuff->sieve_primes_adjust);
  printf("  5  FlushInterval     = %-8u  (f/F)\n", mystuff->flush);
  printf("  6  Verbosity         = %-8u  (v/V)\n", mystuff->verbosity);
  printf("  7  PrintMode         = %-8u  (r/R)\n", mystuff->printmode);
  printf("  8  Kernel            = %s  (k/K)\n", mystuff->stats.kernelname);

  printf("\n  0  Done (continue factoring)\n");
  printf("\n -1  Exit mfakto (q/Q)\n");
}

static void handle_menu(mystuff_t *mystuff)
{
  int choice, new_value;
  char choice_string[10], new_value_string[50];

  for(;;)
  {
    print_menu(mystuff);
    std::cout << "Change setting number: ";
    std::cin >> choice_string;
    choice = atoi(choice_string);
    if (choice == -1)       // quit
    {
      if (mystuff->verbosity > 0) printf("\nExiting\n");
      mystuff->quit++;
      break;
    }
    if (choice == 0) break; // continue

    std::cout << "Change setting number " << choice << " to:";
    std::cin >> new_value_string;
    new_value = atoi(new_value_string);
  }
}

static void validate_sieve_settings(mystuff_t *mystuff)
{
  unsigned int i;

  if (mystuff->gpu_sieving)
  {
    if (mystuff->gpu_sieve_primes < GPU_SIEVE_PRIMES_MIN) mystuff->gpu_sieve_primes = GPU_SIEVE_PRIMES_MIN;
    if (mystuff->gpu_sieve_primes > GPU_SIEVE_PRIMES_MAX) mystuff->gpu_sieve_primes = GPU_SIEVE_PRIMES_MAX;

    mystuff->gpu_sieve_processing_size = ((mystuff->gpu_sieve_processing_size + 4096) / 8192) * 8192;
    if (mystuff->gpu_sieve_processing_size < GPU_SIEVE_PROCESS_SIZE_MIN*1024) mystuff->gpu_sieve_processing_size = GPU_SIEVE_PROCESS_SIZE_MIN*1024;
    if (mystuff->gpu_sieve_processing_size > GPU_SIEVE_PROCESS_SIZE_MAX*1024) mystuff->gpu_sieve_processing_size = GPU_SIEVE_PROCESS_SIZE_MAX*1024;

    mystuff->gpu_sieve_size = ((mystuff->gpu_sieve_size + 512*1024) / 1024 / 1024) * 1024 * 1024;
    if (mystuff->gpu_sieve_size < GPU_SIEVE_SIZE_MIN*1024*1024) mystuff->gpu_sieve_size = GPU_SIEVE_SIZE_MIN*1024*1024;
    if (mystuff->gpu_sieve_size > GPU_SIEVE_SIZE_MAX*1024*1024) mystuff->gpu_sieve_size = GPU_SIEVE_SIZE_MAX*1024*1024;
    if ((i = mystuff->gpu_sieve_size % mystuff->gpu_sieve_processing_size) != 0) // sieve_size must be a multiple of sieve_processing_size
    {
      // can only happen when GPUSieveProcessSize=24 ==> make it divisible by 3
      mystuff->gpu_sieve_size -= i;
      while (mystuff->gpu_sieve_size < GPU_SIEVE_SIZE_MIN*1024*1024)
        mystuff->gpu_sieve_size+= 3*1024*1024;  // make sure it's not too low
    }
    // need to reinit?
  }
  else
  {
    if (mystuff->sieve_primes < SIEVE_PRIMES_MIN) mystuff->sieve_primes = SIEVE_PRIMES_MIN;
    if (mystuff->sieve_primes < SIEVE_PRIMES_MAX) mystuff->sieve_primes = SIEVE_PRIMES_MAX;
#ifndef SIEVE_SIZE_LIMIT
    if (mystuff->sieve_size < 13*17*19*23) mystuff->sieve_size = 13*17*19*23;
#endif
  }

}

static void lower_sieve_primes(mystuff_t *mystuff)
{
  if (mystuff->gpu_sieving)
  {
    mystuff->gpu_sieve_primes = mystuff->gpu_sieve_primes * 7 / 8;
  }
  else
  {
    mystuff->sieve_primes = mystuff->sieve_primes * 7 / 8;
  }
  validate_sieve_settings(mystuff);
}

static void increase_sieve_primes(mystuff_t *mystuff)
{
  if (mystuff->gpu_sieving)
  {
    mystuff->gpu_sieve_primes = mystuff->gpu_sieve_primes * 9 / 8;
  }
  else
  {
    mystuff->sieve_primes = mystuff->sieve_primes * 9 / 8;
  }
  validate_sieve_settings(mystuff);
}

static void lower_sieve_size(mystuff_t *mystuff)
{
  if (mystuff->gpu_sieving)
  {
    mystuff->gpu_sieve_size = mystuff->gpu_sieve_size * 7 / 8;
  }
#ifndef SIEVE_SIZE_LIMIT
  else
  {
    mystuff->sieve_size -=  13*17*19*23;
  }
#endif
  validate_sieve_settings(mystuff);
}

static void increase_sieve_size(mystuff_t *mystuff)
{
  if (mystuff->gpu_sieving)
  {
    mystuff->gpu_sieve_size = mystuff->gpu_sieve_size * 9 / 8;
  }
#ifndef SIEVE_SIZE_LIMIT
  else
  {
    mystuff->sieve_size +=  13*17*19*23;
  }
#endif
  validate_sieve_settings(mystuff);
}

static void lower_sieve_process_size(mystuff_t *mystuff)
{
  if (mystuff->gpu_sieving)
  {
    mystuff->gpu_sieve_processing_size -= 8192;
    validate_sieve_settings(mystuff);
  }
}

static void increase_sieve_process_size(mystuff_t *mystuff)
{
  if (mystuff->gpu_sieving)
  {
    mystuff->gpu_sieve_processing_size += 8192;
    validate_sieve_settings(mystuff);
  }
}

static void use_previous_kernel(mystuff_t *mystuff)
{
}

static void use_next_kernel(mystuff_t *mystuff)
{
}

int handle_kb_input(mystuff_t *mystuff)
{
  int last_key = 0;

  while (_kbhit())
  {
    switch (last_key = _getch())
    {
    case 'm': handle_menu(mystuff);
      break;
    case '-': lower_sieve_primes(mystuff);
      break;
    case '+': increase_sieve_primes(mystuff);
      break;
    case 's': lower_sieve_size(mystuff);
      break;
    case 'S': increase_sieve_size(mystuff);
      break;
    case 'p': lower_sieve_process_size(mystuff);
      break;
    case 'P': increase_sieve_process_size(mystuff);
      break;
    case 'a': if (mystuff->verbosity > 0)
                printf("\nSetting SievePrimesAdjust to 0 (was %u)\n", mystuff->sieve_primes_adjust);
              mystuff->sieve_primes_adjust=0;
      break;
    case 'A': if (mystuff->verbosity > 0)
                printf("\nSetting SievePrimesAdjust to 1 (was %u)\n", mystuff->sieve_primes_adjust);
              mystuff->sieve_primes_adjust=1;
      break;
    case 'f': if (mystuff->flush > 0)
              {
                if (mystuff->verbosity > 0)
                  printf("\nSetting FlushInterval to %u (was %u)\n", mystuff->flush - 1, mystuff->flush);
                mystuff->flush--;
              }
      break;
    case 'F': if (mystuff->verbosity > 0)
                printf("\nSetting FlushInterval to %u (was %u)\n", mystuff->flush + 1, mystuff->flush);
              mystuff->flush++;
      break;
    case 'v': if (mystuff->verbosity > 0)
              {
                mystuff->verbosity--;
                printf("\nSetting Verbosity to %u\n", mystuff->verbosity);
              }
      break;
    case 'V': mystuff->verbosity++;
              if (mystuff->verbosity > 0) printf("\nSetting Verbosity to %u\n", mystuff->verbosity);
      break;
    case 'r': if (mystuff->verbosity > 0)
                printf("\nSetting PrintMode to 0 (was %u)\n", mystuff->printmode);
              mystuff->printmode=0;
      break;
    case 'R': if (mystuff->verbosity > 0)
                printf("\nSetting PrintMode to 1 (was %u)\n", mystuff->printmode);
              mystuff->printmode=1;
      break;
    case 'k': use_previous_kernel(mystuff);
      break;
    case 'K': use_next_kernel(mystuff);
      break;
    case 'q':
    case 'Q': if (mystuff->verbosity > 0)
                printf("\n%c: Exiting\n", last_key);
              mystuff->quit++;
      break;
    default: printf("\nIgnoring unknown keyboard input '%c'\n", last_key);
      break;
    }
  }
  return last_key;
}