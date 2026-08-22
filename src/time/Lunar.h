#pragma once

#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Classic bit-packed lunar table (1900–2049). Base: 1900-01-31 = lunar 1900-1-1.
namespace Lunar {

struct Date {
  int year;
  int month;
  int day;
  bool leap;
};

static const uint32_t kInfo[] = {
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2,
    0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977,
    0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970,
    0x06566, 0x0d4a0, 0x0ea50, 0x06e95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950,
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557,
    0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5d0, 0x14573, 0x052d0, 0x0a9a8, 0x0e950, 0x06aa0,
    0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0,
    0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b5a0, 0x195a6,
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570,
    0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x055c0, 0x0ab60, 0x096d5, 0x092e0,
    0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5,
    0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930,
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530,
    0x05aa0, 0x076a3, 0x096d0, 0x04bd7, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45,
    0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0,
};

inline int leapMonth(int y) { return (int)(kInfo[y - 1900] & 0xF); }

inline int leapDays(int y) {
  if (!leapMonth(y)) {
    return 0;
  }
  return (kInfo[y - 1900] & 0x10000) ? 30 : 29;
}

inline int monthDays(int y, int m) {
  return (kInfo[y - 1900] & (0x10000 >> m)) ? 30 : 29;
}

inline int yearDays(int y) {
  int sum = 348;
  for (uint32_t i = 0x8000; i > 0x8; i >>= 1) {
    if (kInfo[y - 1900] & i) {
      ++sum;
    }
  }
  return sum + leapDays(y);
}

inline int daysBetween(int y1, int m1, int d1, int y2, int m2, int d2) {
  static const int md[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  auto yday = [](int y, int m, int d) {
    int n = d;
    for (int i = 1; i < m; ++i) {
      n += md[i];
      if (i == 2) {
        const bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        if (leap) {
          ++n;
        }
      }
    }
    return n;
  };
  int days = 0;
  for (int y = y1; y < y2; ++y) {
    const bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    days += leap ? 366 : 365;
  }
  return days - yday(y1, m1, d1) + yday(y2, m2, d2);
}

inline bool fromGregorian(const struct tm &t, Date &out) {
  const int y = t.tm_year + 1900;
  const int m = t.tm_mon + 1;
  const int d = t.tm_mday;
  if (y < 1901 || y > 2049) {
    return false;
  }

  int offset = daysBetween(1900, 1, 31, y, m, d);
  int i = 1900;
  int yd = 0;
  while (i < 2050 && offset > 0) {
    yd = yearDays(i);
    offset -= yd;
    ++i;
  }
  if (offset < 0) {
    offset += yd;
    --i;
  }

  const int ly = i;
  const int lm = leapMonth(ly);
  bool isLeap = false;
  int month = 1;
  int md = 0;

  while (month < 13 && offset > 0) {
    if (lm > 0 && month == lm + 1 && !isLeap) {
      --month;
      isLeap = true;
      md = leapDays(ly);
    } else {
      md = monthDays(ly, month);
    }
    offset -= md;
    if (isLeap && month == lm + 1) {
      isLeap = false;
    }
    ++month;
  }
  if (offset < 0) {
    offset += md;
    --month;
  }
  if (offset == 0 && lm > 0 && month == lm + 1) {
    if (isLeap) {
      isLeap = false;
    } else {
      isLeap = true;
      --month;
    }
  }

  out.year = ly;
  out.month = month;
  out.day = offset + 1;
  out.leap = isLeap;
  return true;
}

inline void formatShort(const Date &ld, char *buf, size_t n) {
  if (ld.leap) {
    snprintf(buf, n, "L%d*/%d", ld.month, ld.day);
  } else {
    snprintf(buf, n, "L%d/%d", ld.month, ld.day);
  }
}

}  // namespace Lunar
