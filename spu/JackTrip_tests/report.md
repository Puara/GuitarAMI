# JackTrip tests - Brams/IDMIL/uToronto journal

Report in progress used on the JackTrip tests and hardware assembly using the GuitarAMI SPU hardware for the SingWell partnership.

## October 14th, 2021

Meeting summary:

- We successfully ran JackTrip on the GuitarAMI Sound Processing Unit (SPU). The SPU acted as a server, and we could connect a laptop also running JackTrip and a client.
- The SPU is capable of serving JackTrip, synthesize sounds, or share audio through the network.
- Start discussing next week the strategies to communicate between UdeM and McGill.

## October 28th, 2021

- Preliminary tests on Brams using two [GuitarAMI SPUs](https://github.com/edumeneses/GuitarAMI/blob/jacktrip/docs/SPU_user_guide.md). Setup worked using a local network, the SPUs, and a laptop used to interact with the machines.

## Next Steps

- [X] Duplicate the existing setup (SPU + JackTrip) and automatically set one as a client and another as a server (Oct. 18-22nd, 2021).
- [X] Test latency between the 2 SPUs in a local network on Brams (Oct. 28th, 2021).
- [ ] Measure latency between the 2 SPUs in a local network using jack-delay (Nov. 2nd to 4th, 2021).
- [ ] Test internet latency between the 2 SPUs. One SPU will be located at Brams and another at IDMIL. We will need IT on both universities to fully open UDP communication at port 4464, and we'll probably need to set a VPN between both institutions (TBD, probably Nov. 5th, 2021).
- [ ] Define audio interface for the SingWell SPUs (TBD).
- [ ] Build 2 SPUs for the project (TBD).
- [ ] Mailing one of the SPUs to uToronto and final tests (TBD).
