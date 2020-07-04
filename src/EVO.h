#ifndef EVO_H
#define EVO_H

class EVO {
    public:

        bool SetHeading(double heading);
        bool SetAuto();
        bool SetTrack();
        bool SetVane();
        bool SetStandBy();
        bool SetWind(double wind);
};


#endif