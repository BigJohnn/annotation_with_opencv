/*****************************************************************************
*   Non-Rigid Face Tracking
******************************************************************************
*   by Jason Saragih, 5th Dec 2012
*   http://jsaragih.org/
******************************************************************************
*   Ch6 of the book "Mastering OpenCV with Practical Computer Vision Projects"
*   Copyright Packt Publishing 2012.
*   http://www.packtpub.com/cool-projects-with-opencv/book
*****************************************************************************/
/*
  annotate: annotation tool
  Jason Saragih (2012)
*/
#include "opencv_hotshots/ft/ft.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
#include <map>

//the file name.
//string name = "panda_hp";
string name = "foxMask";

struct lessPoint2f
{
	bool operator()(const Point2f& lhs, const Point2f& rhs) const
	{
		return (lhs.x == rhs.x) ? (lhs.y < rhs.y) : (lhs.x < rhs.x);
	}
};

//==============================================================================
class annotate{
public:
  int idx;                       //index of image to annotate
  int pidx;                      //index of point to manipulate
  Mat image;                     //current image to display 
  Mat image_alpha;
  Mat image_clean;               //clean image to display
  ft_data data;                  //annotation data
  float ratio;
  const char* wname;             //display window name
  vector<string> instructions;   //annotation instructions
  int center_line_x;
  int counter;
  annotate(){wname = "Annotate"; idx = 0; pidx = -1;}

  int
  set_current_image(const int idx = 0){
    if((idx < 0) || (idx > int(data.imnames.size())))return 0;
    image = data.get_image(idx,2); return 1;
  }
  void
  set_clean_image(){
    image_clean = image.clone();
  }
  void
  copy_clean_image(){
    image_clean.copyTo(image);
  }
  void
  draw_instructions(){
    if(image.empty())return;
    this->draw_strings(image,instructions);
  }
  void
  draw_points(){
    data.draw_points(image,idx);
	
	if (data.points[0].size() != 0)
	{
		Point p = data.points[0][data.points[0].size() - 1];

		char buffer[10];
		_itoa_s(counter-1, buffer, 10, 10);
		putText(image, buffer, p, FONT_HERSHEY_COMPLEX, 0.6f,
			Scalar(0,255,0), 1, CV_AA);
	}
  }
  void
	  draw_twin_points(){
	  data.draw_points(image, idx);

	  if (data.points[0].size() != 0)
	  {
		  Point p = data.points[0][data.points[0].size() - 1];

		  char buffer[10];
		  _itoa_s(counter - 1, buffer, 10, 10);
		  putText(image, buffer, p, FONT_HERSHEY_COMPLEX, 0.6f,
			  Scalar(0, 255, 0), 1, CV_AA);
	  }
  }
  void
  draw_chosen_point(){
	  if (pidx >= 0)
	  {
		  if (data.points[0].size() != 0)
		  {
			  Point p = data.points[0][pidx];

			  char buffer[10];
			  _itoa_s(pidx, buffer, 10, 10);
			  putText(image, buffer, p, FONT_HERSHEY_COMPLEX, 0.6f,
				  Scalar(0, 255, 0), 1, CV_AA);
		  }
		  circle(image, data.points[idx][pidx], 1, CV_RGB(0, 255, 0), 2, CV_AA);
	  }
  }
  void
	  draw_all_points(){
		  if (data.points[0].size() != 0)
		  {
			  for (int i = 0; i < data.points[0].size(); i++)
			  {
				  Point p = data.points[0][i];

				  char buffer[10];
				  _itoa_s(i, buffer, 10, 10);
				  putText(image, buffer, p, FONT_HERSHEY_COMPLEX, 0.6f,
					  Scalar(0, 255, 0), 1, CV_AA);
				  circle(image, data.points[idx][i], 1, CV_RGB(255, 0, 0), 2, CV_AA);
			  }
			  
		  }
  }
  void
  draw_connections(){
    int m = data.connections.size();
    if(m == 0)this->draw_points();
    else{
      if(data.connections[m-1][1] < 0){
    int i = data.connections[m-1][0];
    data.connections[m-1][1] = i;
    data.draw_connect(image,idx); this->draw_points();
    circle(image,data.points[idx][i],1,CV_RGB(0,255,0),2,CV_AA);
    data.connections[m-1][1] = -1;
      }else{data.draw_connect(image,idx); this->draw_points();}
    }
  }
  void
  draw_symmetry(){
    this->draw_points(); this->draw_connections();
    for(int i = 0; i < int(data.symmetry.size()); i++){
      int j = data.symmetry[i];
      if(j != i){
    circle(image,data.points[idx][i],1,CV_RGB(255,255,0),2,CV_AA);
    circle(image,data.points[idx][j],1,CV_RGB(255,255,0),2,CV_AA);
      }
    }
    if(pidx >= 0)circle(image,data.points[idx][pidx],1,CV_RGB(0,255,0),2,CV_AA);
  }
  void
  set_capture_instructions(){
    instructions.clear();
    instructions.push_back(string("Select expressive frames."));
    instructions.push_back(string("s - use this frame"));
    instructions.push_back(string("q - done"));
  }
  void
  set_pick_points_instructions(){
    instructions.clear();
    instructions.push_back(string("Pick Points"));
    instructions.push_back(string("q - done"));
  }
  void
  set_connectivity_instructions(){
    instructions.clear();
    instructions.push_back(string("Generate Triangulation"));
    instructions.push_back(string("q - done"));
  }
  void
  set_symmetry_instructions(){
    instructions.clear();
    instructions.push_back(string("Pick Symmetric Points"));
    instructions.push_back(string("q - done"));
  }
  void
  set_move_points_instructions(){
    instructions.clear();
    instructions.push_back(string("Move Points"));
	instructions.push_back(string("d - redoTriangulate"));
    //instructions.push_back(string("p - next image"));
    //instructions.push_back(string("o - previous image"));
    instructions.push_back(string("q - done & save"));
  }
  void
  initialise_symmetry(const int index){
    int n = data.points[index].size(); data.symmetry.resize(n);
    for(int i = 0; i < n; i++)data.symmetry[i] = i;
  }
  void
  replicate_annotations(const int index){
    if((index < 0) || (index >= int(data.points.size())))return;
    for(int i = 0; i < int(data.points.size()); i++){
      if(i == index)continue;
      data.points[i] = data.points[index];
    }
  }
  int
  find_closest_point(const Point2f p,
             const double thresh = 10.0){
    int n = data.points[idx].size(),imin = -1; double dmin = -1;
    for(int i = 0; i < n; i++){
      double d = norm(p-data.points[idx][i]);
      if((imin < 0) || (d < dmin)){imin = i; dmin = d;}
    }
    if((dmin >= 0) && (dmin < thresh))return imin; else return -1;
  }
protected:
  void
  draw_strings(Mat img,
           const vector<string> &text){
    for(int i = 0; i < int(text.size()); i++)this->draw_string(img,text[i],i+1);
  }
  void
  draw_string(Mat img, 
          const string text,
          const int level)
  {
    Size size = getTextSize(text,FONT_HERSHEY_COMPLEX,0.6f,1,NULL);
    putText(img,text,Point(0,level*size.height),FONT_HERSHEY_COMPLEX,0.6f,
        Scalar::all(0),1,CV_AA);
    putText(img,text,Point(1,level*size.height+1),FONT_HERSHEY_COMPLEX,0.6f,
        Scalar::all(255),1,CV_AA);
  }

public:
	void drawDelaunay()
	{
		Mat1f points_s(data.points[0].size(), 2);
		
		for (int i = 0; i < data.points[0].size(); i++)
		{
			points_s[i][0] = data.points[0][i].x;
			points_s[i][1] = data.points[0][i].y;
		}
		vector<Vec6f> triangles, triangles_dst;
		vector<int> indices;
		Mat1b adj_s, adj_d;
		int rows = annotation.image.rows;
		int cols = annotation.image.cols;

		cout << points_s << endl;
		vector<int> triSrc;
		adj_s = delaunay(points_s, rows, cols, triSrc);//NxN邻接关系表

		for (int i = 0; i < points_s.rows; i++)
		{
			int xi = points_s.at<float>(i, 0);
			int yi = points_s.at<float>(i, 1);

			/// Draw the edges
			for (int j = i + 1; j < points_s.rows; j++)
			{
				if (adj_s(i, j))
				{
					annotation.data.connections.push_back(Vec2i(i, j));
				}
			}
		}
	}
private:
	Mat delaunay(const Mat1f& points, int imRows, int imCols, vector<int> &indices)
	{
		map<Point2f, int, lessPoint2f> mappts;

		Mat1b adj_s(points.rows, points.rows, uchar(0));

		/// Create subdiv and insert the points to it
		Subdiv2D subdiv(Rect(0, 0, imCols, imRows));
		for (int p = 0; p < points.rows; p++)
		{
			float xp = points(p, 0);
			float yp = points(p, 1);
			Point2f fp(xp, yp);

			// Don't add duplicates
			if (mappts.count(fp) == 0)
			{
				// Save point and index
				mappts[fp] = p;

				subdiv.insert(fp);
			}
		}

		/// Get the number of edges
		vector<Vec4f> edgeList;
		subdiv.getEdgeList(edgeList);
		int nE = edgeList.size();

		/// Check adjacency
		for (int i = 0; i < nE; i++)
		{
			Vec4f e = edgeList[i];
			Point2f pt0(e[0], e[1]);
			Point2f pt1(e[2], e[3]);

			if (mappts.count(pt0) == 0 || mappts.count(pt1) == 0) {
				// Not a valid point
				continue;
			}

			int idx0 = mappts[pt0];
			int idx1 = mappts[pt1];

			// Symmetric matrix
			adj_s(idx0, idx1) = 1;
			adj_s(idx1, idx0) = 1;
		}

		vector<Vec6f> triangleList;
		subdiv.getTriangleList(triangleList);

		int nT = triangleList.size();
		/// Check triangle
		for (int i = 0; i < triangleList.size(); i++)
		{
			Vec6f t = triangleList[i];
			Point2f pt0(t[0], t[1]);
			Point2f pt1(t[2], t[3]);
			Point2f pt2(t[4], t[5]);
			if (mappts.count(pt0) == 0 || mappts.count(pt1) == 0 || mappts.count(pt2) == 0) {
				triangleList.erase(triangleList.begin() + i, triangleList.begin() + i + 1);
				i--;
				// Not a valid point
				continue;
			}

			int idx0 = mappts[pt0];
			int idx1 = mappts[pt1];
			int idx2 = mappts[pt2];

			indices.push_back(idx0);
			indices.push_back(idx1);
			indices.push_back(idx2);
		}

		return adj_s;
	}
}annotation;
//==============================================================================
void pp_MouseCallback(int event, int x, int y, int /*flags*/, void* /*param*/)
{ 
  if(event == CV_EVENT_LBUTTONDOWN){

		if (annotation.counter < 101)
		{
			annotation.data.points[0].push_back(Point2f(x, y));
			annotation.counter++;
			if (annotation.counter == 10)
				annotation.center_line_x = annotation.data.points[0][9].x;
			annotation.draw_points();
		}
		else if (annotation.counter == 101)
		{
			annotation.data.points[0].push_back(Point2f(annotation.center_line_x, y));
			annotation.counter++;
			annotation.draw_points();
		}
		else if (annotation.counter > 101)
		{
			if (abs(x - annotation.center_line_x) < 10)
			{
				x = annotation.center_line_x;
				annotation.data.points[0].push_back(Point2f(annotation.center_line_x * 2 - x, y));
				annotation.counter++;
				annotation.draw_points();
			}
			else if (annotation.counter >= 171 && annotation.counter <= 178)
			{
				annotation.data.points[0].push_back(Point2f(x, y));
				annotation.counter++;
				annotation.draw_points();
			}
			else
			{
				annotation.data.points[0].push_back(Point2f(x, y));
				annotation.counter++;
				annotation.draw_points();
				annotation.data.points[0].push_back(Point2f(annotation.center_line_x * 2 - x, y));
				annotation.counter++;
				annotation.draw_points();
			}
		}
     imshow(annotation.wname,annotation.image);
  }
}
//==============================================================================
void pc_MouseCallback(int event, int x, int y, int /*flags*/, void* /*param*/)
{ 
  if(event == CV_EVENT_LBUTTONDOWN){
    int imin = annotation.find_closest_point(Point2f(x,y));
    if(imin >= 0){ //add connection
      int m = annotation.data.connections.size();
      if(m == 0)annotation.data.connections.push_back(Vec2i(imin,-1));
      else{
    if(annotation.data.connections[m-1][1] < 0)//1st connecting point chosen
      annotation.data.connections[m-1][1] = imin;
    else annotation.data.connections.push_back(Vec2i(imin,-1));
      }
      annotation.draw_connections(); 
      imshow(annotation.wname,annotation.image); 
    }
  }
}
//==============================================================================
void ps_MouseCallback(int event, int x, int y, int /*flags*/, void* /*param*/)
{ 
  if(event == CV_EVENT_LBUTTONDOWN){
    int imin = annotation.find_closest_point(Point2f(x,y));
    if(imin >= 0){
      if(annotation.pidx < 0)annotation.pidx = imin;
      else{
    annotation.data.symmetry[annotation.pidx] = imin;
    annotation.data.symmetry[imin] = annotation.pidx;
    annotation.pidx = -1;
      }      
      annotation.draw_symmetry(); 
      imshow(annotation.wname,annotation.image); 
    }
  }
}
//=============================================================================
void px_MouseCallback(int event, int x, int y, int /*flags*/, void* /*param*/)
{
	if (event == CV_EVENT_LBUTTONDOWN){
		int imin = annotation.find_closest_point(Point2f(x, y));
		if (imin >= 0){ //add connection
			int m = annotation.data.connections.size();
			if (m == 0)annotation.data.connections.push_back(Vec2i(imin, -1));
			else{
				if (annotation.data.connections[m - 1][1] < 0)//1st connecting point chosen
				{
					int temp = m;
					for (int i = 0; i < m; i++)
					{
						//如果原先有连接关系，抹去。
						if ((annotation.data.connections[i][1] == annotation.data.connections[m - 1][0] && imin == annotation.data.connections[i][0])
							|| (annotation.data.connections[i][0] == annotation.data.connections[m - 1][0] && imin == annotation.data.connections[i][1]))
						{
							annotation.data.connections.erase(annotation.data.connections.begin() + i, annotation.data.connections.begin() + i + 1);
							m -= 1;
							//annotation.set_current_image(0);
							annotation.image = annotation.image_alpha.clone();
						}
					}
					if (temp == m)
					annotation.data.connections[m - 1][1] = imin;
				}
				else
				{
					annotation.data.connections.push_back(Vec2i(imin, -1));
				}
			}
			annotation.draw_connections();
			imshow(annotation.wname, annotation.image);
		}
	}
}
//==============================================================================
void mv_MouseCallback(int event, int x, int y, int /*flags*/, void* /*param*/)
{ 
  if(event == CV_EVENT_LBUTTONDOWN){
    if(annotation.pidx < 0){
      annotation.pidx = annotation.find_closest_point(Point2f(x,y));
    }else annotation.pidx = -1;
    //annotation.copy_clean_image();//image_clean.copyTo(image);
	annotation.image = annotation.image_alpha.clone();
    annotation.draw_connections();
    annotation.draw_chosen_point();
    imshow(annotation.wname,annotation.image);
  }else if(event == CV_EVENT_MOUSEMOVE){
    if(annotation.pidx >= 0){
      annotation.data.points[annotation.idx][annotation.pidx] = Point2f(x,y);
      //annotation.copy_clean_image();
	  annotation.image = annotation.image_alpha.clone();
      annotation.draw_connections();
      annotation.draw_chosen_point();
      imshow(annotation.wname,annotation.image); 
    }
  }
  else if (event == CV_EVENT_LBUTTONUP)
  {
	  annotation.draw_all_points();
	  imshow(annotation.wname, annotation.image);
  }
  
}
//==============================================================================
class muct_data{
public:
  string name;
  string index;
  vector<Point2f> points;

  muct_data(string str,
        string muct_dir){
    size_t p1 = 0,p2;
    
    //set image directory
    string idir = muct_dir; if(idir[idir.length()-1] != '/')idir += "/";
    idir += "jpg/";

    //get image name
    p2 = str.find(",");
    if(p2 == string::npos){cerr << "Invalid MUCT file" << endl; exit(0);}
    name = str.substr(p1,p2-p1);
    
    if((strcmp(name.c_str(),"i434xe-fn") == 0) || //corrupted data
       (name[1] == 'r'))name = "";                //ignore flipped images
    else{
      name = idir + str.substr(p1,p2-p1) + ".jpg"; p1 = p2+1;
      
      //get index
      p2 = str.find(",",p1);
      if(p2 == string::npos){cerr << "Invalid MUCT file" << endl; exit(0);}
      index = str.substr(p1,p2-p1); p1 = p2+1;
      
      //get points
      for(int i = 0; i < 75; i++){
    p2 = str.find(",",p1);
    if(p2 == string::npos){cerr << "Invalid MUCT file" << endl; exit(0);}
    string x = str.substr(p1,p2-p1); p1 = p2+1;
    p2 = str.find(",",p1);
    if(p2 == string::npos){cerr << "Invalid MUCT file" << endl; exit(0);}
    string y = str.substr(p1,p2-p1); p1 = p2+1;
    points.push_back(Point2f(atoi(x.c_str()),atoi(y.c_str())));
      }
      p2 = str.find(",",p1);
      if(p2 == string::npos){cerr << "Invalid MUCT file" << endl; exit(0);}
      string x = str.substr(p1,p2-p1); p1 = p2+1;
      string y = str.substr(p1,str.length()-p1);
      points.push_back(Point2f(atoi(x.c_str()),atoi(y.c_str())));
    }
  }
};
//==============================================================================
bool
parse_help(int argc,char** argv)
{
  for(int i = 1; i < argc; i++){
    string str = argv[i];
    if(str.length() == 2){if(strcmp(str.c_str(),"-h") == 0)return true;}
    if(str.length() == 6){if(strcmp(str.c_str(),"--help") == 0)return true;}
  }return false;
}
//==============================================================================
string 
parse_odir(int argc,char** argv)
{
  string odir = "data/";
  for(int i = 1; i < argc; i++){
    string str = argv[i];
    if(str.length() != 2)continue;
    if(strcmp(str.c_str(),"-d") == 0){
      if(argc > i+1){odir = argv[i+1]; break;}
    }
  }
  if(odir[odir.length()-1] != '/')odir += "/";
  return odir;
}
//==============================================================================
int 
parse_ifile(int argc,
        char** argv,
        string& ifile)
{
  for(int i = 1; i < argc; i++){
    string str = argv[i];
    if(str.length() != 2)continue;
    if(strcmp(str.c_str(),"-m") == 0){ //MUCT data
      if(argc > i+1){ifile = argv[i+1]; return 2;}
    }
    if(strcmp(str.c_str(),"-v") == 0){ //video file
      if(argc > i+1){ifile = argv[i+1]; return 1;}
    }
  }
  ifile = ""; return 0;
}
//==============================================================================
int main(int argc,char** argv)
{
  //parse cmd line options
  if(parse_help(argc,argv)){
    cout << "usage: ./annotate [-v video] [-m muct_dir] [-d output_dir]" 
     << endl; return 0;
  }
  string odir = parse_odir(argc,argv);
  string ifile; int type = parse_ifile(argc,argv,ifile);
  string fname = odir + "annotations.yaml"; //file to save annotation data to

  type = 3;
  //get data
  namedWindow(annotation.wname, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
  if(type == 2){ //MUCT data
    string lmfile = ifile + "muct76-opencv.csv";
    ifstream file(lmfile.c_str()); 
    if(!file.is_open()){
      cerr << "Failed opening " << lmfile << " for reading!" << endl; return 0;
    }
    string str; getline(file,str);
    while(!file.eof()){
      getline(file,str); if(str.length() == 0)break;
      muct_data d(str,ifile); if(d.name.length() == 0)continue;
      annotation.data.imnames.push_back(d.name);
      annotation.data.points.push_back(d.points);
    }
    file.close();
    annotation.data.rm_incomplete_samples();
  }
  else if (type == 3)
  {
	  setMouseCallback(annotation.wname, pp_MouseCallback, 0);
	  annotation.set_pick_points_instructions();
	  annotation.data.imnames.push_back(name+".png");
	  annotation.image = imread(name+".png",-1);
	  if (1600 / annotation.image.cols >= 900 / annotation.image.rows)
	  {
		  annotation.ratio = 900.0 / annotation.image.rows;
	  }
	  else
	  {
		  annotation.ratio = 1600.0 / annotation.image.cols;
	  }
	  resize(annotation.image, annotation.image, Size(annotation.image.cols*annotation.ratio, annotation.image.rows*annotation.ratio));
	  uchar color[3] = { 200, 200, 190 };
	  for (int i = 0; i < annotation.image.cols; i++)
	  {
		  uchar* data = annotation.image.ptr<uchar>(i);
		  for (int j = 0; j < annotation.image.rows; j++)
		  {
			  float alpha = data[4 * j + 3] * 1.0 / 255;
			  data[4 * j] = data[4 * j] * alpha + color[0] * (1 - alpha);
			  data[4 * j+1] = data[4 * j+1] * alpha + color[1] * (1 - alpha);
			  data[4 * j+2] = data[4 * j+2] * alpha + color[2] * (1 - alpha);
		  }
	  }
	  annotation.image_alpha = annotation.image.clone();
	  //imwrite(name + "resized.png", annotation.image);
	  annotation.draw_instructions();
	  annotation.idx = 0;
	  annotation.data.points.resize(1);
	  while (1){
		  annotation.draw_points();
		  imshow(annotation.wname, annotation.image);
		  if (waitKey(0) == 'q')
		  {
			  imwrite("data\\" + name + "_marked.png", annotation.image);
			  break;
		  }
	  }
	  
	  if (annotation.data.points[0].size() == 0)return 0;
	  annotation.replicate_annotations(0);
  }
  else
  {
    //open video stream
    VideoCapture cam; 
	if (type == 1)
	{
		cam.open(ifile);
	}
	else
	{
		cam.open(0);
	}
    if(!cam.isOpened())
	{
      cout << "Failed opening video file." << endl
       << "usage: ./annotate [-v video] [-m muct_dir] [-d output_dir]" 
       << endl; return 0;
    }
    //get images to annotate
    annotation.set_capture_instructions();
    while(cam.get(CV_CAP_PROP_POS_AVI_RATIO) < 0.999999)
	{
		  Mat im,img; cam >> im; annotation.image = im.clone(); 
		  annotation.draw_instructions();
		  imshow(annotation.wname,annotation.image); int c = waitKey(10);
		  if(c == 'q')break;
		  else if(c == 's')
		  {
			int idx = annotation.data.imnames.size(); char str[1024];
			if     (idx < 10)sprintf(str,"%s00%d.png",odir.c_str(),idx);
			else if(idx < 100)sprintf(str,"%s0%d.png",odir.c_str(),idx);
			else               sprintf(str,"%s%d.png",odir.c_str(),idx);
			imwrite(str,im); annotation.data.imnames.push_back(str);
			im = Scalar::all(255); imshow(annotation.wname,im); waitKey(10);
		  }
   }
    if(annotation.data.imnames.size() == 0)return 0;
    annotation.data.points.resize(annotation.data.imnames.size());

    //annotate first image
    setMouseCallback(annotation.wname,pp_MouseCallback,0);
    annotation.set_pick_points_instructions();
    annotation.set_current_image(0);
    annotation.draw_instructions();
    annotation.idx = 0;
	
    while(1){ annotation.draw_points();
      imshow(annotation.wname,annotation.image); if(waitKey(0) == 'q')break;
    }
    if(annotation.data.points[0].size() == 0)return 0;
    annotation.replicate_annotations(0);
  }
  save_ft(fname.c_str(),annotation.data);
  
  //annotate connectivity
  setMouseCallback(annotation.wname,pc_MouseCallback,0);
  annotation.image = annotation.image_alpha.clone();
  annotation.set_connectivity_instructions();
  //annotation.set_current_image(0);
  annotation.draw_instructions();
  annotation.idx = 0;
  
  while(1){ 
	  annotation.drawDelaunay();
	  annotation.draw_connections(); 
      imshow(annotation.wname,annotation.image); if(waitKey(0) == 'q')break;
  }
  save_ft(fname.c_str(),annotation.data); 

  //annotate symmetry
  //setMouseCallback(annotation.wname,ps_MouseCallback,0);
  //annotation.initialise_symmetry(0);
  //annotation.set_symmetry_instructions();
  //annotation.set_current_image(0);
  //annotation.draw_instructions();
  //annotation.idx = 0; annotation.pidx = -1;
  //while(1){ annotation.draw_symmetry(); 
  //  imshow(annotation.wname,annotation.image); if(waitKey(0) == 'q')break;
  //}
  //save_ft(fname.c_str(),annotation.data); 

  //annotate the rest
  if(type != 2){
    setMouseCallback(annotation.wname,mv_MouseCallback,0);
    annotation.set_move_points_instructions();
    annotation.idx = 0;//此处原来是1
	annotation.pidx = -1;
    while(1){
      annotation.set_current_image(annotation.idx);
	  annotation.image = annotation.image_alpha.clone();
      annotation.draw_instructions();
	  //annotation.set_clean_image(); //image_clean = image.clone();
      annotation.draw_connections();
	  annotation.draw_all_points();
      imshow(annotation.wname,annotation.image);
      int c = waitKey(0);
      if     (c == 'q')break;
      else if(c == 'p'){annotation.idx++; annotation.pidx = -1;}
      else if(c == 'o'){annotation.idx--; annotation.pidx = -1;}
	  else if (c == 'd') { 
		  annotation.data.connections.clear();
		  annotation.drawDelaunay(); }
      if(annotation.idx < 0)annotation.idx = 0;
      if(annotation.idx >= int(annotation.data.imnames.size()))
    annotation.idx = annotation.data.imnames.size()-1;
    }
  }

  setMouseCallback(annotation.wname,px_MouseCallback,0);
  annotation.initialise_symmetry(0);
  annotation.set_symmetry_instructions();
  //annotation.set_current_image(0);
  annotation.image = annotation.image_alpha.clone();
  annotation.draw_instructions();
  annotation.idx = 0; annotation.pidx = -1;
  while(1){ annotation.draw_symmetry(); 
    imshow(annotation.wname,annotation.image); if(waitKey(0) == 'q')break;
  }
  save_ft(fname.c_str(),annotation.data); 

  //vector<Vec2i> cs = annotation.data.connections;
  //int length = cs.size();
  //for (int i = 0; i < length; i++)
  //{
	 // Point2f start = annotation.data.points[0][cs[i][0]];
	 // Point2f end = annotation.data.points[0][cs[i][1]];
	 // //Point2f median = (start + end) / 2;
	 // 
	 // annotation.data.points[0].push_back((start + end) / 2);
	 // //median.x = annotation.data.points[cs[i][0]]
  //}
  vector<Point2f> points = annotation.data.points[0];
  FileStorage fs("data\\" +name +"_vertices.yml", FileStorage::WRITE);

  fs << "feaCoord" << "[:";
  for (int i = 0; i < points.size(); i++)
  {
	  points[i].x /= annotation.ratio;
	  points[i].y /= annotation.ratio;
	  fs << (int)points[i].x << (int)points[i].y;
  }
  fs << "]";

  fs.release();

  save_ft(fname.c_str(),annotation.data); destroyWindow("Annotate"); return 0;
}
//==============================================================================
