DGS-1210-52 defaults:
    IP:    10.90.90.90
    Login: admin
    Pass:  admin
    Reset: press "Reset button" for ~6 seconds

Basic configuration:
    config ipif System ipaddress 10.90.90.X 255.255.255.0 gateway 10.90.90.254
    config account admin password wsfy6g1remd49aug
    save

VLANs:
    101-131 - TEAMS (31 = FAKE) 10.60.N.0/24  (N: 1..31)
    200     - ORGS              10.10.10.0/24
    300     - INTERNET          ?

Ports:
    01..40  - TEAMS 01..20
    41,42   - TEAM 31 (FAKE)
    43,44   - ORGS
    45,46   - INTERNET
    47,48   - ROUTER
    49..52  - MANAGEMENT (VLAN 1)
