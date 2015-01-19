 
 
 
 
 
 
 
 
 
 
 	forAll(mesh_.boundary(),i)
	{
	  			if(mesh_.boundary()[i].type()=="wall")
			{
            
                scalarField &refomegaboundary = omega_.boundaryField()[i];	
                			  
                scalarField &refGboundary = G.boundaryField()[i];	
                			  
                scalarField magUboundary = mag(U_.boundaryField()[i].snGrad());	
                	
                scalarField squareutau = magUboundary*nu();
                
                scalarField distancefluid =  nearWallDist(mesh_).y()[i];
                
                scalarField yplusw = distancefluid/nu()*sqrt(squareutau)*yplusfactor;
                
                scalarField ksmin = min(2.4* pow(yplusw,0.85),8.0);  // 2.4 - Orig
                
                scalar ks = 1e-15;
                
                scalarField SRwall= sqr(50/(max(ksmin,ks)));
                
                refomegaboundary = squareutau/nu()*SRwall;
                
                omega_.boundaryField()[i].patchInternalField()=squareutau/nu()*SRwall;
                
                //Info << refomegaboundary << endl;
                }  
			
	} 
 
 
 
 
 
 

 
 
 
