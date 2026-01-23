// snatchforce_core.cpp

#define _USE_MATH_DEFINES

#include "snatchforce_core.h"
#include <cmath>
#include <numbers>
#include <stdexcept>

double computeDensity(double altitude_m)
{
    // Simple ISA troposphere approximation
    // ρ = ρ0 * (1 - L*h/T0)^{(g/(R*L) - 1)}
    const double term = 1.0 - (LAPSERATE * altitude_m) / T0;
    if(term <= 0.0)
        return 0.0;
    const double exponent = (G / (R * LAPSERATE)) - 1.0;
    return RHO0 * std::pow(term, exponent);
}

double computeProjectedArea(double diameter_m)
{
    const double r = diameter_m * 0.5;
    return M_PI * r * r;
}

double computeDynamicPressure(double rho, double v)
{
    return 0.5 * rho * v * v;
}

double computeTerminalVelocity(double mass,
                               double rho,
                               double CoD_drogue,
                               double Sproj_drogue)
{
    // Vt = sqrt[(2 * m * g) / (rho * CoD_drogue * Sproj_drogue)]
    if (rho <= 0.0 || CoD_drogue <= 0.0 || Sproj_drogue <= 0.0)
        return 0.0;
    return std::sqrt((2.0 * mass * G) / (rho * CoD_drogue * Sproj_drogue));
}

static void computeForces(const CkInput& ck,
                          double q,
                          double CoD,
                          double Sproj,
                          ChuteResult& out)
{
    out.Fsteady = q * CoD * Sproj;
    out.ckUsed = ck;

    if(ck.useExact)
    {
        out.FmaxExact = ck.exact * q * CoD * Sproj;
        out.FmaxMin = out.FmaxMax = out.FmaxEst = 0.0;
    }
    else
    {
        out.FmaxMin = ck.ckMin * q * CoD * Sproj;
        out.FmaxMax = ck.ckMax * q * CoD * Sproj;
        out.FmaxEst = ck.ckEst * q * CoD * Sproj;
        out.FmaxExact = 0.0;
    }
}

ChuteResult computeChute(const ChuteInput& input,
                         double rocketMass,
                         const ChuteResult* drogueForAutoVel)
{
    ChuteResult res{};

    // Unit conversion to SI
    rocketMass    = input.imperial ? rocketMass      / 2.205 : rocketMass;
    res.mass      = input.imperial ? input.mass      / 2.205 : input.mass;
    res.deployAlt = input.imperial ? input.deployAlt / 3.281 : input.deployAlt;
    res.deploySpeed = input.imperial ? input.deploySpeed / 2.237 : input.deploySpeed;
    res.chuteD    = input.imperial ? input.chuteD    / 3.281 : input.chuteD;
    res.CoD       = input.CoD;
    


    // Atmospheric and geometry
    res.rho   = computeDensity(res.deployAlt);
    res.Sproj = computeProjectedArea(res.chuteD);

    // Auto velocity (for main/payload) based on drogue terminal at this altitude
    if(input.useAutoVelocity && drogueForAutoVel)
    {
        res.deploySpeed = computeTerminalVelocity(
            rocketMass,
            res.rho,
            drogueForAutoVel->CoD,
            drogueForAutoVel->Sproj
        );
    }

    // Dynamic pressure and forces
    res.q = computeDynamicPressure(res.rho, res.deploySpeed);
    computeForces(input.ck, res.q, res.CoD, res.Sproj, res);

    return res;
}
