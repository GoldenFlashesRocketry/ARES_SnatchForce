// snatchforce_core.h
#pragma once

#include <cmath>

constexpr double RHO0 = 1.225;   // kg/m^3     sea-level density
constexpr double LAPSERATE = 0.0065; // K/m   lapse rate
constexpr double T0 = 288.15;    // K         sea-level temp
constexpr double G = 9.80665;    // m/s^2     gravity
constexpr double R = 287.05;     // J/(kg*K)  specific gas constant for air

struct CkInput
{
    bool useExact = true;  // true = use exact Ck; false = use range
    double exact = 1.0;    // if useExact
    double ckMin = 1.0;    // lower bound
    double ckMax = 1.0;    // upper bound
    double ckEst = 1.0;    // estimate
};

struct ChuteInput
{
    bool imperial = false;     // true = (lb, ft, mph, ft); false = SI
    double mass = 0.0;         // rocket/payload mass
    double deployAlt = 0.0;    // deployment altitude
    double deploySpeed = 0.0;  // speed at deployment; 0.0 means "auto" for main/payload
    double CoD = 0.0;          // chute drag coefficient
    double chuteD = 0.0;       // parachute diameter
    bool useAutoVelocity = false; // for main/payload: compute from drogue terminal
    CkInput ck;
};

struct ChuteResult
{
    // Inputs converted to SI
    double mass = 0.0;        // kg
    double deployAlt = 0.0;   // m
    double deploySpeed = 0.0; // m/s
    double CoD = 0.0;
    double chuteD = 0.0;      // m

    // Derived
    double rho = 0.0;         // kg/m^3
    double q = 0.0;           // dynamic pressure
    double Sproj = 0.0;       // projected area
    double Fsteady = 0.0;     // q * CoD * Sproj

    // Snatchforce results
    CkInput ckUsed;
    double FmaxExact = 0.0;   // if useExact
    double FmaxMin = 0.0;     // if using range
    double FmaxMax = 0.0;
    double FmaxEst = 0.0;
};

double computeDensity(double altitude_m);
double computeProjectedArea(double diameter_m);
double computeDynamicPressure(double rho, double v);
double computeTerminalVelocity(double mass,
                               double rho,
                               double CoD_drogue,
                               double Sproj_drogue);

ChuteResult computeChute(const ChuteInput& input,
                         double rocketMass,
                         const ChuteResult* drogueForAutoVel = nullptr);
