#pragma once
#include <algorithm>
#include <glm/gtx/norm.hpp>


inline float cross(const glm::vec2& a, const glm::vec2& b) {
	return a.x * b.y - a.y * b.x;
}

inline bool point_in_rect(const glm::vec2& p, glm::vec2 r1, glm::vec2 r2) {
	if (r1.x > r2.x) std::swap(r1.x, r2.x);
	if (r1.y > r2.y) std::swap(r1.y, r2.y);
	return p.x >= r1.x && p.x <= r2.x && p.y >= r1.y && p.y <= r2.y;
}

inline bool on_same_side(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& d) {
	glm::vec2 p = d - c;
	glm::vec2 ca = a - c;
	glm::vec2 cb = b - c;
	float l = cross(p, ca);
	float m = cross(p, cb);
	return l * m >= 0;
}

inline bool point_in_triangle(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
	return on_same_side(p, a, b, c) && on_same_side(p, b, a, c) && on_same_side(p, c, a, b);
}

inline float point_to_line_segment_distance(const glm::vec2& p, glm::vec2 l1, glm::vec2 l2) {
	glm::vec2 pl = p - l1;
	glm::vec2 ll = l2 - l1;
	float t = std::max(0.0f, std::min(1.0f, glm::dot(pl, ll) / glm::length2(ll)));
	return glm::distance(p, l1 + t * ll);
}

inline bool is_oriented_cw(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
	glm::vec2 ab = b - a;
	glm::vec2 ac = c - a;
	return cross(ac, ab) >= 0;
}

inline bool any_point_in_triangle(
		const std::vector<const glm::vec2*>& vertices,
		const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
	for (const glm::vec2* p : vertices) {
		if ((p != &a) && (p != &b) && (p != &c) && point_in_triangle(*p, a, b, c)) return true;
	}
	return false;
}

inline bool is_ear(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const std::vector<const glm::vec2*>& vertices) {
	return is_oriented_cw(a, b, c) && !any_point_in_triangle(vertices, a, b, c);
}

template <class Func>
bool triangulate(const std::vector<glm::vec2>& poly, Func f) {
	std::vector<int> prev_index(poly.size());
	std::vector<int> next_index(poly.size());
	for (int i = 0; i < (int) poly.size(); ++i) {
		prev_index[i] = i - 1;
		next_index[i] = i + 1;
	}
	prev_index[0] = poly.size() - 1;
	next_index[poly.size() - 1] = 0;

	std::vector<const glm::vec2*> concave;
	for (int i = 0; i < (int) poly.size(); ++i) {
		if (!is_oriented_cw(poly[prev_index[i]], poly[i], poly[next_index[i]])) {
			concave.push_back(&poly[i]);
		}
	}

	int left = poly.size();
	int skipped = 0;
	int current = 1;
	int next, prev;
	while (left > 3) {
		prev = prev_index[current];
		next = next_index[current];
		const glm::vec2& a = poly[prev];
		const glm::vec2& b = poly[current];
		const glm::vec2& c = poly[next];
		if (is_ear(a, b, c, concave)) {
			f(a, b, c);
			next_index[prev] = next;
			prev_index[next] = prev;
			auto it = std::find(concave.begin(), concave.end(), &b);
			if (it != concave.end()) concave.erase(it);
			--left;
			skipped = 0;
		}
		else if (++skipped > left) {
			printf("WOOT\n");
			return false;
		}
		current = next;
	}
	next = next_index[current];
	prev = prev_index[current];
	f(poly[prev], poly[current], poly[next]);

	return true;
}
