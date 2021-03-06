#ifndef _MESH_FACTORY_HH_
#define _MESH_FACTORY_HH_

#include "Data.hh"
#include "Field_data.hh"
#include "Mesh_STK_Impl.hh"
#include "Data_structures.hh"
#include "GeometricModel.hh"

// Trilinos STK_mesh includes.
#include <Shards_BasicTopologies.hpp>
#include <stk_util/parallel/Parallel.hpp>
#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/GetBuckets.hpp>
#include <stk_mesh/base/FieldData.hpp>
// #include <stk_mesh/fem/FieldDeclarations.hpp>
#include <stk_mesh/fem/FEMHelpers.hpp>
#include <stk_mesh/fem/FEMMetaData.hpp>

#include <Epetra_MpiComm.h>


namespace Amanzi {
namespace AmanziMesh {

namespace Data {
class Data;
class Element_block;
class Side_set;
class Node_set;
} // close namespace Data

namespace STK {

/*!
 * Class which builds a Mesh_STK_Impl and fields from a Data::Data
 * and Data::Fields specification.
 */
class Mesh_STK_factory {
 private:

  const int bucket_size_;
  stk::ParallelMachine parallel_machine_;

  const Epetra_MpiComm *communicator_;


  typedef std::vector<stk::mesh::Part*>   Parts;

  typedef std::map<Entity_Ids, stk::mesh::Entity*> Vector_entity_map;

  void add_coordinates_ (const Data::Coordinates<double>& data, 
                         const Epetra_Map& vertmap);
  void build_meta_data_ (const Data::Data& data, const Data::Fields& fields,
                         const Teuchos::RCP<const AmanziGeometry::GeometricModel>& gm);
  void build_bulk_data_ (const Data::Data& data, 
                         const Epetra_Map& cellmap, 
                         const Epetra_Map& vertmap,
                         const Data::Fields& fields,
                         const Teuchos::RCP<const AmanziGeometry::GeometricModel>& gm);

  // Add parts to the meta-data.
  stk::mesh::Part* add_element_block_ (const Data::Element_block& block);
  stk::mesh::Part* add_element_block_ (const std::string name, const int id);
  stk::mesh::Part* add_side_set_      (const Data::Side_set& set);
  stk::mesh::Part* add_side_set_      (const std::string name, const int id);
  stk::mesh::Part* add_node_set_      (const Data::Node_set& set);
  stk::mesh::Part* add_node_set_      (const std::string name, const int id);

  // Populate parts with elements and fields via the bulk-data
  void add_elements_to_part_ (const Data::Element_block& block, stk::mesh::Part& part,
                              const Epetra_Map& cellmap, const Epetra_Map& vertmap, unsigned int& localidx0);
  void add_sides_to_part_    (const Data::Side_set& side_set,   stk::mesh::Part& part,
                              const Epetra_Map& cellmap);
  void add_nodes_to_part_    (const Data::Node_set& node_set,   stk::mesh::Part& part,
                              const Epetra_Map& vertmap);

  void put_field_ (const Data::Field& field, stk::mesh::Part&, unsigned int space_dimension);
  void put_coordinate_field_ (stk::mesh::Part& part, unsigned int space_dimension);

  void add_set_part_relation_ (unsigned int set_id, stk::mesh::Part& part);

  const stk::mesh::Entity&
  declare_face_(stk::mesh::EntityVector& nodes, 
                const unsigned int& index,
                const unsigned int& owner_index);
    
  /// Generate the mesh faces local to this processor
  int generate_local_faces_(const int& fidx0, const bool& justcount = false);

  /// Generate cell-to-face relations
  void generate_cell_face_relations(void);

  /// Count the number of faces that need to be generated on this processor
  int count_local_faces_(void) { return generate_local_faces_(0, true); }

  /// Check, and change if necessary, side set face ownership
  void check_face_ownership_(void);

  /// Get the set of nodes defining the specified element side
  Entity_vector
  get_element_side_nodes_(const stk::mesh::Entity& element, const unsigned int& s);

  /// Get the face declared for the specified cell side, if any
  stk::mesh::Entity *
  get_element_side_face_(const stk::mesh::Entity& element, const unsigned int& s);
    

  // Build extra part info from the geometric model
  void init_extra_parts_from_gm(const Teuchos::RCP<const AmanziGeometry::GeometricModel>& gm);
  void fill_extra_parts_from_gm(const Teuchos::RCP<const AmanziGeometry::GeometricModel>& gm);

  // Temporary information for the mesh currently under construction.

  stk::mesh::BulkData *bulk_data_;
  stk::mesh::fem::FEMMetaData *meta_data_;
  Entity_map          *entity_map_;


  stk::mesh::EntityRank node_rank_;
  stk::mesh::EntityRank face_rank_;
  stk::mesh::EntityRank element_rank_;
    
  int face_id_;
  Parts element_blocks_;
  Parts side_sets_;
  Parts node_sets_;

  stk::mesh::Part* nodes_part_;
  stk::mesh::Part* faces_part_;
  stk::mesh::Part* elements_part_;

  Vector_field_type *coordinate_field_;
  Id_field_type *face_owner_;

  Id_map set_to_part_;

 public:

  Mesh_STK_factory (const Epetra_MpiComm *comm_, int bucket_size);

  //! Build a mesh from data.
  Mesh_STK_Impl* build_mesh (const Data::Data& data, 
                             const Data::Fields& fields,
                             const Teuchos::RCP<const AmanziGeometry::GeometricModel>& gm);

  //! Build a mesh from data with global indexes specified.
  Mesh_STK_Impl* build_mesh (const Data::Data& data, 
                             const Epetra_Map& cellmap,
                             const Epetra_Map& vertmap,
                             const Data::Fields& fields,
                             const Teuchos::RCP<const AmanziGeometry::GeometricModel>& gm);

};

} // close namespace STK 
} // close namespace Mesh 
} // close namespace Amanzi 


#endif
