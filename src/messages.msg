//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

// Message definitions for TSN Leader Election

// Leader message for arbitrary network election
packet LeaderMsg {
    int senderId;
    int leaderValue;
    int roundNum;
}

// Bit message for anonymous network election
packet BitMsg {
    int senderId;
    int bitValue;
    int roundNum;
    bool isActive;
}

// Leader announcement message
packet LeaderAnnouncement {
    int leaderId;
}
