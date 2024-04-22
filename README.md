@[toc]
# 【图形学】【Ray Tracing in One Weekend】总结及相关问题的原因

## 前言

本文为e-book, "Ray Tracing in One Weekend" by Peter Shirley, 的总结。
主要内容包括：简易光线追踪的实现，漫反射材质的实现，金属材质的实现，电解质材质的实现，相机的实现，以及实验过程中遇到一些问题和原因

最终的效果如下：
![final scene image](https://img-blog.csdnimg.cn/direct/0d90b3144bd34e9385254727fe5f411b.png#pic_center)
## 简易光线追踪的实现

我们从相机（cam）处，屏幕（image panel）中的每一个像素点发射一条光线，检测其是否与场景中的物体发生碰撞，得到交点；
根据物体的材质，决定光线在该处如何散射（包括反射或折射）；
跟踪散射光线，继续求交，直到达到最大弹射次数或者与场景中的物体没有交点。

图片如下：
![在这里插入图片描述](https://img-blog.csdnimg.cn/direct/e9b5a9abe9ce4c398d4cbba5f97d710f.gif#pic_center)

代码如下(In Camera.hpp)：

```cpp    

void render(const hittable& world) {
    initialize();

    std::cout << "P3\n"
                << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; ++j) { // antialiasing
        std::clog << "\rScanlines remaining: " << (image_height - 1 - j) << " "
                    << std::flush;
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);
            for (int sample = 0; sample < samples_per_pixel; ++sample) {
                ray r = get_ray(i, j);
                pixel_color += ray_color(r, max_depth, world);
            }
            write_color(std::cout, pixel_color * pixel_samples_scale);
        }
    }
    std::clog << ("\nDone.\n");
}
```

跟踪散射光线，递归求解；下面为背景（天空）的颜色，用线性插值来过渡
```cpp
color ray_color(const ray& r, int depth, const hittable& world) {
    if (depth <= 0)
        return color(0, 0, 0);  // all abosorb
    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        ray scattered;
        color attenuation;
        if (rec.mat->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, depth - 1, world);
        return color(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    color start_color = color(1.0, 1.0, 1.0);
    color end_color = color(0.5, 0.7, 1.0);
    double a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * start_color + a * end_color;  // linear interpolation
}
```

对于物体覆盖问题，即一条光线与多个物体有交点，屏幕返回什么颜色。我们可以用for循环遍历每个物体，用tmp_rec记录下来，然后取离相机最近的物体即可。

代码如下（In hittable_list.hpp）：

```cpp
bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    hit_record tmp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;

    for (const auto& object : objects) {
        if (object->hit(r, interval(ray_t.min, closest_so_far), tmp_rec)) {
            hit_anything = true;
            closest_so_far = tmp_rec.t;
            // std::clog << closest_so_far << "*\n";
            rec = tmp_rec;
        }
    }
    return hit_anything;
}
```
                                                                                                       
## 漫反射材质的实现 

- 性质：当光达到材质表面时，光线等概率地向四周反射
- 如图所示：
- ![](https://img-blog.csdnimg.cn/direct/3e2ca58a161e494081bae07a0a52c9fd.jpeg#pic_center)- 朗伯特余弦定律（Lambert's Cosine Law）： 在理想的漫反射模型中，辐射强度与观察者的视角与表面法线之间夹角的余弦成正比；$I = I_0 \cos \theta$
- 如图所示：
- ![在这里插入图片描述](https://img-blog.csdnimg.cn/direct/0ae0cff2aa32433690688b4ca3e2398d.jpeg#pic_center)
-  代码如下

```cpp
class lambertian : public material {
   public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 scatter_direction = rec.normal + random_unit_vector();
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

   private:
    color albedo;
};
```

- 效果如下：
- ![](https://img-blog.csdnimg.cn/direct/d18c10128ccf4818a96a1ec08d2f3071.png#pic_center)
## 金属材质的实现

- 在金属材质中，我们模拟的是镜面反射和金属表面的模糊
- 参考图片如下：
- ![](https://img-blog.csdnimg.cn/direct/a281c669ac5b4d01ba7f59fed9ad4052.jpeg#pic_center)- ![](https://img-blog.csdnimg.cn/direct/cc9670fcc9394f779736a651f4eb4dbe.jpeg#pic_center)- 渲染图片：
- ![](https://img-blog.csdnimg.cn/direct/10b7aea1fc1c4f3dadfbb4c6b67f1235.png#pic_center)- ![](https://img-blog.csdnimg.cn/direct/6a5d6f5013154c8baa5f251835a61e8f.png#pic_center)
- 在镜面反射，入射光线、出射光线与平面法线的夹角相同
- 至于模糊，设置模糊度fuzz，再出射光线加上一个fuzz * random_unit_vector()
- 代码如下：

```cpp
class metal : public material {
   public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1){};
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

   private:
    color albedo;
    double fuzz;
};
```

## 电介质材质的实现

- 光线打到电介质物体表面，不仅会发生反射，而且会发射折射
- 斯涅尔定律（Snell's Law）: 
	- $n_1 \sin \theta_1 = n2 \sin \theta_2$
	- ![](https://img-blog.csdnimg.cn/direct/3dd01b3274e346dab37a358c83553bec.png#pic_center) 我们知道，如果我们正视玻璃，它可能会变成一面镜子，即存在反射。这里我们用 Schilick 近似（Schlick Approximation）来表示
- Schilick Approximation 
	- $R(\theta) = R_0+(1-R_0)(1-\cos \theta)^5$
	- $R_0 = (\frac{n_1-n_2}{n_1+n_2})^2$
	- $R$ 是镜面反射系数

- 参考图片：
![](https://img-blog.csdnimg.cn/direct/85708187e4b946949ec78d640d6b73d2.jpeg#pic_center)
- 渲染图片：
![](https://img-blog.csdnimg.cn/direct/513a22db8b8e4ecea96e967e82320b9c.png#pic_center)

代码如下：

```cpp
class dielectric : public material {
   public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        attenuation = color(1.0, 1.0, 1.0);  // transparent
        double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        return true;
    }

   private:
    double refraction_index;

    static double reflectance(double cosine, double refraction_index) {
        // Schilick's approximation
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
};
```

## 相机的实现

- 建立观察坐标系，提供aspect_ratio, vfov, lookfrom, lookat等参数的设置
- 观察坐标系如下图：
- ![](https://img-blog.csdnimg.cn/direct/6b33db708590407d82cb8266f4922a13.jpeg#pic_center) 代码如下（In Camera.hpp）：

```cpp
class camera {
   public:
    double aspect_ratio = 1.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;  // maximum number of ray bounces into scene

    double vfov = 90;  // angle
    point3 lookfrom = point3(0, 0, 0);
    point3 lookat = point3(0, 0, -1);
    vec3 vup = vec3(0, 1, 0);
    // ......
}
```

```cpp
 // cacualte u, v, w
 w = unit_vector(lookfrom - lookat);
 u = unit_vector(cross(vup, w));
 v = cross(w, u);
```

- 

## Q&A
### 什么是伽马校正（Gamma Correction）?

- 相机捕捉到的图像和人眼看到的图像存在亮度上的差异，下图分别是相机捕捉到的图像和人眼看到的图像![相机捕捉到的图像](https://img-blog.csdnimg.cn/direct/aff5a21fe78d4483beb97d46bba15887.webp#pic_center) ![人眼看到的图像](https://img-blog.csdnimg.cn/direct/883bbeae13d64434b369b38c6d3385c4.webp#pic_center)-![](https://img-blog.csdnimg.cn/direct/5e211ae77d1b4799b6c9c8168acd6646.png#pic_center)因为我们的眼睛对暗部细节特别敏感，是为了让我们在光线不足的情况下，依然有辨别危险的能力
- 在CRT设备中，显示器自动进行了伽马校正以便显示正确的颜色（业界统一标准的gamma值为2.2）
- ![](https://img-blog.csdnimg.cn/direct/bf3130094264439eaefc7606824a69ce.png#pic_center)- 对于以及正规化的颜色（$color \in [0, 1]$），我们可以对其开根来调亮图片


代码如下（In Color.hpp）：

```cpp
void write_color(std::ostream& out, const color& pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // the inverse of gamma correction
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    static const interval intensity(0.000, 0.999);
    int rbyte = int(255.999 * intensity.clamp(r));
    int gbyte = int(255.999 * intensity.clamp(g));
    int bbyte = int(255.999 * intensity.clamp(b));

    out << rbyte << " " << gbyte << " " << bbyte << "\n";
}
```



### 为什么球体渲染到屏幕中变成椭圆了？

- 问题描述：在我们做金属材质的时候我们向场景中添加了3个球体，代码如下：
```cpp
    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left   = make_shared<metal>(color(0.8, 0.8, 0.8));
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2));

    world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.2),   0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
```

``
但是当我们渲染到屏幕是，左右的球体变成椭圆了，如图：
![在这里插入图片描述](https://img-blog.csdnimg.cn/direct/0a1ce9f310d94f2b97b485a9a6b38080.png#pic_center)为什么？
-	 对于场景中的每一个物体，以相机中心为视点，光线沿着球的轮廓切过，我们可以得到一个圆锥。在本例中，我们可以得到3个圆锥
-	物体投影到平面的过程，等同于拿image panel与各个圆锥相交，输出相交平面，存在四种情况，分别形成圆（Cirle）、椭圆（Ellipse），抛物线（Parabola），双曲线（Hyperbola）
-	![](https://img-blog.csdnimg.cn/direct/5f6ade1c168f4336be41f9b360d69900.png#pic_center)



### 为什么会发生散焦模糊（Defocus Blur, Defocus aberration）？
![](https://img-blog.csdnimg.cn/direct/733576cc62104ffe9ed6f485867b6006.png#pic_center)- 
- 对于从边缘来的光，其焦点可能在image sensor之前，倒像之间相互重叠模糊
-  ![](https://img-blog.csdnimg.cn/direct/feaf11dbd8d74a43aa908badcbcfb7a0.webp#pic_center)
### The Whole  Code
- 
## 参考资料
- [Ray Tracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html#diffusematerials)
- [gamma-correction](https://www.cambridgeincolour.com/tutorials/gamma-correction.htm)
- [Does a sphere projected into 2D space always result in an ellipse?](https://computergraphics.stackexchange.com/questions/1493/does-a-sphere-projected-into-2d-space-always-result-in-an-ellipse)
- [Why does a bigger aperture causes "blurry" background?](https://www.quora.com/Why-does-a-bigger-aperture-causes-blurry-background)
