/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn, int flit_id, bool is_modified,  GarnetNetwork* p, flit* t_flit)
{
    int outport = -1;


    if (route.dest_router == m_router->get_id()) {

        std::vector<std::string> directions = t_flit -> get_direction();
        std::vector<int> routers = t_flit -> get_path();

        for(int i = 0; i < 16; i++){
            Router *tempRouter = p->getRouter(i);
            std:: cout << "Router number : " << i << "\n";
            std::cout << "before increasing trust now : \n";
            tempRouter->print_trusts();

        }

        assert(directions.size() + 1 == routers.size());

        for(int i = 0; i < routers.size()-1; i++){

            Router *tempRouter = p->getRouter(routers[i]);

            std::cout << "\n increasing for router : " << routers[i] << "and direction : " << directions[i]  << " \n";

            if(directions[i] == "North"){
                tempRouter->increment_north_trust();
            }
            if(directions[i] == "South"){
                tempRouter -> increment_south_trust();
            }

            if(directions[i] == "West"){
                tempRouter -> increment_west_trust();
            }

            if(directions[i] == "East"){
                tempRouter -> increment_east_trust();
            }
            
        }

        for(int i = 0; i < 16; i++){
            Router *tempRouter = p->getRouter(i);
            std:: cout << "Router number : " << i << "\n";
            std::cout << "after increasing trust now : \n";
            tempRouter->print_trusts();
        }

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    std::cout << "\nhere in modified routing algorithm\n";
    outport = outportComputeDXY(route, inport, inport_dirn, flit_id, t_flit);

    return outport;

        
}

int RoutingUnit::getRoutingUnitNumber(int router_no, PortDirection outport_dirn, int num_cols){
    if(outport_dirn == "North"){
        return router_no + num_cols; 
    }

    if(outport_dirn == "East"){
        return router_no + 1;
    }

    if(outport_dirn == "South"){
        return router_no - num_cols;
    }

    if(outport_dirn == "West"){
        return router_no - 1;
    }

    return -1;

}


int
RoutingUnit:: outportComputeDXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn, int flit_id, flit* t_flit)
{
    PortDirection outport_dirn = "Unknown";
    

    int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = (dest_x - my_x);
    int y_hops = (dest_y - my_y);

    long double mx;
    vector<int> vect;
    int l, r, t, b;
    int sta=2;
    
       
    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    int src_id = route.src_router;
    int src_x = src_id % num_cols;
    int src_y = src_id / num_cols;

    // 0 -> west
    // 1 -> east
    // 2 -> north
    // 3 -> south

    if(x_hops == 0 || y_hops == 0)
    {
        if(x_hops == 0){
            
            if(y_hops > 0){
                outport_dirn = "North";
                m_router -> decrement_north_trust();
                
                // return 2;
                }
            else if(y_hops < 0){
                outport_dirn = "South";
                 m_router -> decrement_south_trust();
                // return 3;
                }
            }
        else if(y_hops == 0){
            
            if(x_hops > 0){
                outport_dirn = "East";
                m_router -> decrement_east_trust();
                //return 1;
            }
            else if(x_hops < 0){
                outport_dirn = "West";
                m_router -> decrement_west_trust();

            }
        }
    }
    else
    { //NSS

        std::cout << "\nHere comparing directions !!!!\n";

        if(x_hops > 0)          
        { //syam


            if(y_hops > 0)
                {
               
                    
                        // vect.push_back(0);
                        // vect.push_back(1);


                        double north_value = m_router->get_north_trust(); 
                        double east_value = m_router->get_east_trust(); 



                        // assert(north_router >=0 && north_router <16);
                        // assert(east_router >=0 && east_router <16);
                                
    
            
                        if(north_value > east_value){                            
                            outport_dirn = "North";
                            m_router -> decrement_north_trust();
                        }
                        else {
                            outport_dirn = "East";
                            m_router -> decrement_east_trust();
                        }
           
                        // vect.clear();

                }      
                
            
            else if(y_hops < 0)
                {
                



                        // assert(south_router >=0 && south_router <16);
                        // assert(east_router >=0 && east_router <16);


                        double south_value = m_router->get_south_trust(); 
                        double east_value = m_router->get_east_trust(); 

            
            
                       if(south_value > east_value){
                            outport_dirn = "South";
                            m_router -> decrement_south_trust();
                       }
                        else {
                            outport_dirn = "East";
                            m_router -> decrement_east_trust();
                        }


                        // vect.clear();

                }

                    
        }//syam
        
        else if(x_hops < 0)

        { //sankar

            if(y_hops > 0)
                    {
                
                    

                  


                        // vect.push_back(0);
                        // vect.push_back(1);
           

            
                       // int randompos= rand()%vect.size();

                        int north_router = getRoutingUnitNumber(my_id, "North", num_cols);
                        int west_router = getRoutingUnitNumber(my_id, "West", num_cols);   

                        // assert(north_router >=0 && north_router <16);
                        // assert(west_router >=0 && west_router <16);

                        double north_value = m_router->get_north_trust(); 
                        double west_value = m_router->get_west_trust(); 


                        // if(direction == "North") return 0;
                        // if(direction == "East") return 1;
                        // if(direction == "South") return 2;
                        // if(direction == "West") return 3;
         
            
                         if(north_value > west_value){
                            outport_dirn = "North";
                            m_router -> decrement_north_trust();
                         }
                        else {
                            outport_dirn = "West";
                            m_router -> decrement_west_trust();
                        }


                      //  vect.clear();
                    }
                    
              
            
            else if(y_hops < 0)
                    {
                
                    
                    
                        // vect.push_back(0);
                        // vect.push_back(1);
           

            
                        // int randompos= rand()%vect.size();

                        int west_router = getRoutingUnitNumber(my_id, "West", num_cols);
                        int south_router = getRoutingUnitNumber(my_id, "South", num_cols); 

                        // assert(west_router >=0 && west_router <16);
                        // assert(south_router >=0 && south_router <16);

                        double west_value = m_router->get_west_trust();
                        double south_value = m_router->get_south_trust(); 



                        // if(direction == "North") return 0;
                        // if(direction == "East") return 1;
                        // if(direction == "South") return 2;
                        // if(direction == "West") return 3;
           
            
                         if(west_value > south_value){
                            outport_dirn = "West";
                            m_router -> decrement_west_trust();
                         }
                        else {
                            outport_dirn = "South";
                            m_router -> decrement_south_trust();
                        }


                       // vect.clear();
                    }
                    
                    
        }//sankar


    }//NSS

    t_flit -> add_to_direction(outport_dirn);
    return m_outports_dirn2idx[outport_dirn];
}







int
RoutingUnit::outportComputeXYModified(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn, int flit_id)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    //std :: cout << "\n\n\nNumber of columns : " << num_cols << "\n"; 
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();


    cout << "\nRouter id : " << my_id << "\n";
    cout << "flit_id : " << flit_id << "\n";
    cout << "Input port direction: " << inport_dirn << "\n";
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);


    cout << "my x : " << my_x << "\n";
    cout << "my y : " << my_y << "\n";
    cout << "destination_id : " << dest_id << "\n";
    cout << "destination x : " << dest_x << "\n";
    cout << "destination y : " << dest_y << "\n";
    cout << "x hops : " << x_hops << "\n";
    cout << "y hops : " << y_hops << "\n";
    cout << "x direction : " << x_dirn << "\n";
    cout << "y direction : " << y_dirn << "\n";

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            //assert(inport_dirn == "Local" || inport_dirn == "West");
           // std:: cout << "Going to East\n";
            outport_dirn = "East";
        } else {
            // assert(inport_dirn == "Local" || inport_dirn == "East");
            //std:: cout << "Going to West\n";
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            //assert(inport_dirn != "North");
            //std:: cout << "Going to North\n";
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            //assert(inport_dirn != "South");
            //std:: cout << "Going to South\n";
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}




// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn, int flit_id)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    // std :: cout << "\n\n\nNumber of columns : " << num_cols << "\n"; 
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();


    // cout << "Router id : " << my_id << "\n";
    // cout << "flit_id : " << flit_id << "\n";
    // cout << "Input port direction: " << inport_dirn << "\n";
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);


    // cout << "my x : " << my_x << "\n";
    // cout << "my y : " << my_y << "\n";
    // cout << "destination_id : " << dest_id << "\n";
    // cout << "destination x : " << dest_x << "\n";
    // cout << "destination y : " << dest_y << "\n";
    // cout << "x hops : " << x_hops << "\n";
    // cout << "y hops : " << y_hops << "\n";
    // cout << "x direction : " << x_dirn << "\n";
    // cout << "y direction : " << y_dirn << "\n";

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            //std:: cout << "Going to East\n";
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            //std:: cout << "Going to West\n";
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            //std:: cout << "Going to North\n";
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
           // std:: cout << "Going to South\n";
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
int
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    panic("%s placeholder executed", __FUNCTION__);
}

} // namespace garnet
} // namespace ruby
} // namespace gem5
