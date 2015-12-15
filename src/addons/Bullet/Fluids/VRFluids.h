#ifndef VRFLUIDS_H_INCLUDED
#define VRFLUIDS_H_INCLUDED

#include "../Particles/VRParticles.h"

OSG_BEGIN_NAMESPACE;

class VRFluids : public VRParticles {

    public:
        enum SimulationType { SPH, XSPH };

        VRFluids();
        VRFluids(bool spawnParticles);
        ~VRFluids();
        static shared_ptr<VRFluids> create();

        void update(int from, int to) override;
        void updateSPH(int from, int to);
        void updateXSPH(int from, int to);

        void setSimulation(SimulationType t, bool forceChange=false);
        void setSphRadius(float newRadius, float variation);

    protected:
        VRUpdatePtr fluidFkt;
        SimulationType simulation = SPH;

        const bool afterBullet = false;
        const float PRESSURE_KAPPA = N* 8.4 /*1.38*/ *296.0; // ideal gas law: N*(gas const)*(temperature)
        const float PRESSURE_REST  = 0.0001;
        const float VISCOSITY_MU   = 0.01;

        // @depricated!
        inline float calc_density(SphParticle* n, float distance2, float area);
        inline btVector3 xsph_calc_movement(SphParticle* p, SphParticle* n);

        inline float kernel_poly6(float distance2, float area);
        inline float kernel_spiky(float distance, float area);
        inline btVector3 kernel_spiky_gradient(btVector3 r, float h);
        inline float kernel_visc(float distance, float area);
        inline float kernel_visc_laplacian(float distance, float area);

        inline void sph_calc_density(SphParticle* p, btVector3 p_origin, int from, int to);
        inline void sph_calc_pressure(SphParticle* p, btVector3 p_origin, int from, int to);
        inline void sph_calc_viscosity(SphParticle* p, btVector3 p_origin, int from, int to);

        void setFunctions(int from, int to) override;
};

OSG_END_NAMESPACE;

#endif // VRFLUIDS_H_INCLUDED
