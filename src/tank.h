#pragma once

void setupTank();

void toggleTankMode();
String getTankMode();

unsigned long getTankAverage();
void addToTankAverage(unsigned long tankTotal);
void clearAverage();

bool isTankEmpty();