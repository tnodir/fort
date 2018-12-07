#ifndef UTIL_H
#define UTIL_H

#define is_hour_between(hour,from,to) \
  (from <= to ? (hour >= from && hour < to) \
              : (hour >= from || hour < to))

#endif UTIL_H
