#include <position_model.h>


void
PositionModel::readDataFromFolders (std::string path, int number_samples, int number_vertices)
{
  std::string tester("Tester_"),full_path,last_part("/Blendshape/shape_0.obj"),line,value;
  size_t index;
  int i,j,k;
  number_faces_ = number_samples;

  for(i = 1; i <= number_samples; ++i)
  {
    full_path = path + tester + boost::lexical_cast<std::string>(i) + last_part;
    std::ifstream ins(full_path.c_str());

    number_points_ = 11510;

    Eigen::VectorXd S(number_points_ * 3);

    if(ins.fail())
    {
      PCL_ERROR("Could not open file %s\n", full_path.c_str());
      exit(1);
    }

    j = 0;

    while(1)
    {
      std::getline(ins,line);

      std::stringstream ss(line);
      std::string s;
      double d;

      ss >> s;
      if(s.compare("v") != 0)
        break;

      ss >> d;
      S[j++] = d;

      ss >> d;
      S[j++] = d;

      ss >> d;
      S[j++] = d;



    }

    if(i == 1)
    {
      while(line[0] != 'f')
      {
        std::getline(ins,line);
      }

      while(line[0] == 'f' && !ins.eof())
      {
        pcl::Vertices mesh;

        for(k = 0; k < number_vertices; ++k)
        {
          index = line.find(' ');
          line = line.substr(index+1);
          index = line.find('/');
          value = line.substr(0,index);
          mesh.vertices.push_back(boost::lexical_cast<u_int32_t>(value));
        }

        meshes_.push_back(mesh);
        std::getline(ins,line);
      }
    }

    faces_position_cordiantes_.push_back(S);

  }



}

void
PositionModel::readDataFromFolders (std::string path, int number_samples, int number_vertices, Eigen::Matrix3d transformation_matrix, Eigen::Vector3d translation)
{
  std::string tester("Tester_"),full_path,last_part("/Blendshape/shape_0.obj"),line,value;
  size_t index;
  int i,j,k;
  number_faces_ = number_samples;

  for(i = 1; i <= number_samples; ++i)
  {
    full_path = path + tester + boost::lexical_cast<std::string>(i) + last_part;
    std::ifstream ins(full_path.c_str());

    number_points_ = 11510;

    Eigen::VectorXd S(number_points_ * 3);

    if(ins.fail())
    {
      PCL_ERROR("Could not open file %s\n", full_path.c_str());
      exit(1);
    }

    j = 0;

    while(1)
    {
      std::getline(ins,line);

      std::stringstream ss(line);
      std::string s;


      Eigen::Vector3d eigen_point;

      ss >> s;
      if(s.compare("v") != 0)
        break;

      ss >> eigen_point[0];

      ss >> eigen_point[1];

      ss >> eigen_point[2];

      eigen_point = transformation_matrix * eigen_point;
      eigen_point = eigen_point + translation;

      S[j++] = eigen_point[0];
      S[j++] = eigen_point[1];
      S[j++] = eigen_point[2];


    }

    if(i == 1)
    {
      while(line[0] != 'f')
      {
        std::getline(ins,line);
      }

      while(line[0] == 'f' && !ins.eof())
      {
        pcl::Vertices mesh;

        for(k = 0; k < number_vertices; ++k)
        {
          index = line.find(' ');
          line = line.substr(index+1);
          index = line.find('/');
          value = line.substr(0,index);
          mesh.vertices.push_back(boost::lexical_cast<u_int32_t>(value));
        }

        meshes_.push_back(mesh);
        std::getline(ins,line);
      }
    }

    faces_position_cordiantes_.push_back(S);

  }



}


Eigen::VectorXd
PositionModel::calculateMeanFace (bool write)
{
  int i;

  mean_face_positions_ = Eigen::VectorXd::Zero(3 * number_points_);

  for(i = 0; i < number_faces_; ++i)
  {
    mean_face_positions_ += faces_position_cordiantes_[i];
  }

  mean_face_positions_ /= static_cast <double> (number_faces_);



  PCL_INFO("Done with average face\n");

  if(!write)
    return mean_face_positions_;

  std::ofstream ofs;
  ofs.open("PCA.txt", std::ofstream::out | std::ofstream::app);

  ofs << mean_face_positions_.rows() << std::endl;


  for(int i = 0; i < mean_face_positions_.rows(); ++i)
    ofs << mean_face_positions_(i) << ' ';

  ofs << std::endl << std::endl;

  ofs.close();


  return mean_face_positions_;


}


void
PositionModel::calculateEigenValuesAndVectors ()
{
  Eigen::MatrixXd T(3 * number_points_, number_faces_), T_tT;
  int i,j;
  Eigen::VectorXd v;

  for(i = 0; i < T.cols(); ++i)
  {
    T.col(i) = faces_position_cordiantes_[i] - mean_face_positions_;
  }

  T_tT = (T.transpose() * T) / static_cast<double> (number_faces_) ;

  Eigen::EigenSolver <Eigen::MatrixXd> solver(T_tT);

  Eigen::MatrixXd eigenvectors_matrix (T.rows(),solver.eigenvectors().cols());

  for(i = 0; i < solver.eigenvalues().rows(); ++i)
  {
    eigenvalues_vector_.push_back((solver.eigenvalues()[i]).real());
  }

  for(i = 0; i < solver.eigenvectors().cols(); ++i)
  {
    v = T * ((solver.eigenvectors().col(i)).real());
    v.normalize();
    eigenvectors_matrix.col(i) = v;
    eigenvectors_vector_.push_back(v);
  }

  eigenvectors_ = eigenvectors_matrix;
  eigenvalues_ = solver.eigenvalues().real().cast<double>();


}

void
PositionModel::readWeights (std::string file_path)
{
  std::ifstream ifs(file_path.c_str());
  double d;
  if(ifs.fail())
  {
    PCL_ERROR ("No file for weights\n");
    exit(1);
  }

  while(!ifs.eof())
  {
    ifs >> d;
    weights_vector_.push_back(d);
  }


}


void
PositionModel::calculateModel ()
{
  int i;
  face_position_model_ = mean_face_positions_;

  for(i = 0; i < weights_vector_.size(); ++i)
    face_position_model_ += weights_vector_[i] * eigenvectors_vector_[i];
}



void
PositionModel::calculateRandomWeights (int number_samples, std::string name)
{
  int i;
  double w,d;
  std::ofstream ofs_1((std::string("random_") + name + std::string(".txt")).c_str()), ofs_2((std::string("weights_") + name + std::string(".txt")).c_str());

  srand (time(NULL));

  for(i = 0; i < number_samples; ++i)
  {
    d = (double)rand() / RAND_MAX;
    w = d * 4.0 - 2.0;
    ofs_1 << w <<std::endl;

    weights_vector_.push_back(w * eigenvalues_vector_[i]);

    ofs_2 << weights_vector_[i] << std::endl;
  }


}

void
PositionModel::viewModel (int index)
{
  int i;
  Eigen::VectorXd positions;

  if(index == -1)
    positions = face_position_model_;
  else
    positions = faces_position_cordiantes_[index];

  boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
  viewer->setBackgroundColor (0, 0, 0);

  pcl::PointXYZ basic_point;
  pcl::PointCloud<pcl::PointXYZ>::Ptr basic_cloud_ptr (new pcl::PointCloud<pcl::PointXYZ>);

  for(i = 0; i < 3 * number_points_;)
  {
    basic_point.x = static_cast<float>(positions(i++));
    basic_point.y = static_cast<float>(positions(i++));
    basic_point.z = static_cast<float>(positions(i++));

    basic_cloud_ptr->points.push_back(basic_point);
  }

  std::string polygon("polygon");

  viewer->addPolygonMesh <pcl::PointXYZ> (basic_cloud_ptr,meshes_,polygon,0);

  viewer->addCoordinateSystem (1.0, 0);
  viewer->initCameraParameters ();

  while (!viewer->wasStopped ())
  {
    viewer->spinOnce (100);
    boost::this_thread::sleep (boost::posix_time::microseconds (100000));
  }


}

void
PositionModel::writeModel (std::string path)
{

  std::ofstream ofs((path + ".obj").c_str());

  int i,j;

  for(i = 0; i < face_position_model_.rows(); ++i)
  {
    if(i % 3 == 0)
      ofs << "v ";
    ofs << face_position_model_(i);

    if(i % 3 == 2)
      ofs << std::endl;
    else
      ofs << ' ';

  }

  for(i = 0; i < meshes_.size(); ++i)
  {
    ofs << 'f';

    for(j = 0; j < meshes_[i].vertices.size(); ++j)
    {
      ofs<< ' ' << meshes_[i].vertices[j];
    }

    ofs << std::endl;
  }

  ofs.close();
}

void
PositionModel::writeMeanFaceAndRotatedMeanFace(Eigen::MatrixX3d rotation_matrix, Eigen::Vector3d translation_vector, std::string mean_path, std::string transformed_path, pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr source_points, pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr target_points)
{
  int i,j;
  std::ofstream ofs_mean_face(mean_path.c_str());
  std::ofstream ofs_transformed_mean_face(transformed_path.c_str());

  pcl::PointXYZRGBNormal point;

  for( i = 0; i < mean_face_positions_.rows(); i+=3)
  {
    point.x = mean_face_positions_(i);
    point.y = mean_face_positions_(i+1);
    point.z = mean_face_positions_(i+2);

    ofs_mean_face << "v " << point.x << ' ' << point.y << ' ' << point.z << std::endl;

    source_points->push_back(point);
/*
    point[0] = face_position_model_(i);
    point[1] = face_position_model_(i+1);
    point[2] = face_position_model_(i+2);


    transformed_point = rotation_matrix * point + translation_vector;

    target_points.push_back(transformed_point);


    ofs_transformed_mean_face << "v " << transformed_point[0] << ' ' << transformed_point[1] << ' ' << transformed_point[2] << std::endl;
*/
  }

  std::vector < std::vector < Eigen::Vector3f > > normal_vector(source_points->points.size());
  std::vector < std::vector < float > > surface_vector(source_points->points.size());


  for(i = 0; i < meshes_.size();++i)
  {
    Eigen::Vector3f V1,V2,normal;
    V1 = source_points->points[meshes_[i].vertices[1]].getVector3fMap() - source_points->points[meshes_[i].vertices[0]].getVector3fMap();
    V2 = source_points->points[meshes_[i].vertices[2]].getVector3fMap() - source_points->points[meshes_[i].vertices[0]].getVector3fMap();

    normal = V1.cross(V2);

    float area = 0.5 * normal.norm();

    for(j = 0; j < meshes_[i].vertices.size(); ++j)
    {
      normal_vector[meshes_[i].vertices[j]].push_back(normal);
      surface_vector[meshes_[i].vertices[j]].push_back(area);
    }

  }

  for( i = 0; i < source_points->points.size(); ++i)
  {
    float ratio;
    Eigen::Vector3f normal_result;

    ratio = surface_vector[i][0] + surface_vector[i][1] + surface_vector[i][2];

    normal_result = ( ( surface_vector[i][0] * normal_vector[i][0] ) + ( ( surface_vector[i][1] * normal_vector[i][1] ) + ( surface_vector[i][2] * normal_vector[i][2] ) ) ) / ratio;

    source_points->points[i].normal_x = normal_result[0];
    source_points->points[i].normal_y = normal_result[1];
    source_points->points[i].normal_z = normal_result[2];
  }


  Eigen::Matrix4d homogeneus_transform;

  homogeneus_transform.block(0, 0, 3, 3) = rotation_matrix;
  homogeneus_transform.block(0, 3, 3, 1) = translation_vector;
  homogeneus_transform.row(3) << 0, 0, 0, 1;

  pcl::transformPointCloudWithNormals(*source_points,*target_points,homogeneus_transform);

  for(i = 0; i < target_points->points.size(); ++i)
  {
    ofs_transformed_mean_face << "v " << target_points->points[i].x << ' ' << target_points->points[i].y << ' ' << target_points->points[i].z << std::endl;
  }


  for(i = 0; i < meshes_.size(); ++i)
  {
    ofs_mean_face << 'f';
    ofs_transformed_mean_face << 'f';

    for(j = 0; j < meshes_[i].vertices.size(); ++j)
    {
      ofs_mean_face<< ' ' << meshes_[i].vertices[j];
      ofs_transformed_mean_face<< ' ' << meshes_[i].vertices[j];
    }

    ofs_mean_face << std::endl;
    ofs_transformed_mean_face << std::endl;
  }



  ofs_mean_face.close();
  ofs_transformed_mean_face.close();
}


Eigen::VectorXd
PositionModel::getEigenValues(bool write)
{
  if(!write)
    return eigenvalues_;

  std::ofstream ofs;
  ofs.open("PCA.txt", std::ofstream::out | std::ofstream::app);

  ofs << eigenvalues_.rows() << std::endl;


  for(int i = 0; i < eigenvalues_.rows(); ++i)
    ofs << eigenvalues_(i) << ' ';

  ofs << std::endl << std::endl;

  ofs.close();

  return eigenvalues_;
}

Eigen::MatrixXd
PositionModel::getEigenVectors(bool write)
{
  if(!write)
    return eigenvectors_;

  std::ofstream ofs;
  ofs.open("PCA.txt", std::ofstream::out | std::ofstream::app);

  ofs << eigenvectors_.rows() << ' ' << eigenvectors_.cols() << std::endl;

  for(int j = 0; j < eigenvectors_.cols(); ++j)
  {
    for(int i = 0; i < eigenvectors_.rows(); ++i)
    {
      ofs << eigenvectors_(i,j) << ' ';
    }

    ofs << std::endl;
  }

  std::cout << std::endl;

  ofs.close();

  return eigenvectors_;

}

std::vector <  pcl::Vertices >
PositionModel::getMeshes(bool write)
{
  if(!write)
    return meshes_;


  std::ofstream ofs;
  ofs.open("PCA.txt", std::ofstream::out | std::ofstream::app);

  for(int i = 0; i < meshes_.size(); ++i)
  {
    for(int j = 0; j < meshes_[i].vertices.size(); ++j)
    {
      ofs << meshes_[i].vertices[j] << ' ';
    }

    ofs << std::endl;
  }

  ofs << std::endl;

  ofs.close();

  return meshes_;

}

