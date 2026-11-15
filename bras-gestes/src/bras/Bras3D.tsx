/*
  LE BRAS ET LA MAIN EN 3D
  Un modèle construit pièce par pièce, comme un vrai bras robotique de laboratoire :
  socle tournant, montants en aluminium, servomoteurs noirs, main imprimée en 3D à 5 doigts.

  Chaque articulation est un « groupe » 3D qui tourne autour de son axe : quand on tourne un groupe,
  tout ce qui est accroché dedans suit (comme ton bras : tourner l'épaule emporte le coude et la main).
  Les angles arrivent ~60 fois par seconde par « angles.current » : on les applique directement
  dans useFrame, sans redessiner React (beaucoup plus fluide).

  Conventions (les mêmes que le vrai robot) :
    base 90° = face à toi · épaule 90° = vertical · coude 180° = bras tendu · poignet 90° = paume face à toi
    doigts : de « ouvert » (min) à « fermé » (max), voir reglages.ts
*/
import { Environment, Lightformer, OrbitControls, RoundedBox } from "@react-three/drei";
import { Canvas, useFrame, useThree, type ThreeElements } from "@react-three/fiber";
import { useEffect, useMemo, useRef, type MutableRefObject, type ReactNode } from "react";
import * as THREE from "three";
import { DOIGTS, LIMITES, type Angles, type Doigt } from "./reglages";

const rad = THREE.MathUtils.degToRad;

/** Pliage d'un doigt de 0 (ouvert) à 1 (fermé), d'après l'angle de son servo. */
function fermeture(a: Angles, d: Doigt): number {
  const l = LIMITES[d];
  const t = (a[d] - l.min) / (l.max - l.min);
  return Math.min(1, Math.max(0, l.inverse ? 1 - t : t));
}

// ---------- Les matériaux (rendu physique : métal, plastique, impression 3D) ----------
function useMateriaux() {
  return useMemo(
    () => ({
      aluminium: new THREE.MeshStandardMaterial({ color: "#d2d6dc", metalness: 0.85, roughness: 0.34 }),
      aluminiumSombre: new THREE.MeshStandardMaterial({ color: "#9aa0a8", metalness: 0.8, roughness: 0.42 }),
      acier: new THREE.MeshStandardMaterial({ color: "#e4e6ea", metalness: 1, roughness: 0.18 }),
      servo: new THREE.MeshStandardMaterial({ color: "#1d1f23", metalness: 0.15, roughness: 0.55 }),
      socle: new THREE.MeshStandardMaterial({ color: "#2a2c31", metalness: 0.35, roughness: 0.5 }),
      // Main : plastique imprimé en 3D, mat et légèrement chaud
      main: new THREE.MeshPhysicalMaterial({ color: "#efe9df", metalness: 0, roughness: 0.62, clearcoat: 0.15, clearcoatRoughness: 0.6 }),
      etiquette: new THREE.MeshStandardMaterial({ color: "#d9693a", metalness: 0.1, roughness: 0.45 }),
      caoutchouc: new THREE.MeshStandardMaterial({ color: "#3a3d42", metalness: 0, roughness: 0.9 }),
    }),
    [],
  );
}
type Materiaux = ReturnType<typeof useMateriaux>;

/** Un servomoteur : boîtier noir, pattes de fixation, étiquette, et l'axe qui tourne. */
function Servo({ m, taille = [0.62, 0.42, 0.32] as [number, number, number], ...props }: { m: Materiaux; taille?: [number, number, number] } & ThreeElements["group"]) {
  const [l, h, p] = taille;
  return (
    <group {...props}>
      <RoundedBox args={[l, h, p]} radius={0.025} smoothness={3} material={m.servo} castShadow receiveShadow />
      <RoundedBox args={[l + 0.22, 0.04, p]} radius={0.01} position={[0, h * 0.18, 0]} material={m.servo} castShadow />
      <mesh position={[0, 0, p / 2 + 0.002]} material={m.etiquette}>
        <planeGeometry args={[l * 0.55, h * 0.28]} />
      </mesh>
      {[-1, 1].map((s) => (
        <mesh key={s} position={[s * (l / 2 + 0.07), h * 0.18 + 0.025, 0]} material={m.acier}>
          <cylinderGeometry args={[0.022, 0.022, 0.02, 12]} />
        </mesh>
      ))}
    </group>
  );
}

/** Un axe d'articulation : cylindre en acier avec deux roulements. */
function Axe({ m, longueur, rayon = 0.07, ...props }: { m: Materiaux; longueur: number; rayon?: number } & ThreeElements["group"]) {
  return (
    <group {...props}>
      <mesh rotation-z={Math.PI / 2} material={m.acier} castShadow>
        <cylinderGeometry args={[rayon, rayon, longueur, 24]} />
      </mesh>
      {[-1, 1].map((s) => (
        <mesh key={s} position={[(s * longueur) / 2, 0, 0]} rotation-z={Math.PI / 2} material={m.aluminiumSombre} castShadow>
          <cylinderGeometry args={[rayon * 1.9, rayon * 1.9, 0.05, 28]} />
        </mesh>
      ))}
    </group>
  );
}

/** Un doigt : 3 phalanges, chacune pivote à sa base (axe métallique) pour se plier vers la paume. */
function Doigt3D({ m, longueurs, largeur, phalanges }: { m: Materiaux; longueurs: number[]; largeur: number; phalanges: MutableRefObject<(THREE.Group | null)[]> }) {
  const construire = (k: number): ReactNode => {
    if (k >= longueurs.length) return null;
    const l = longueurs[k];
    const fin = k === longueurs.length - 1;
    return (
      <group ref={(g) => { phalanges.current[k] = g; }}>
        <mesh rotation-z={Math.PI / 2} material={m.acier} castShadow>
          <cylinderGeometry args={[0.03, 0.03, largeur + 0.04, 14]} />
        </mesh>
        <RoundedBox args={[largeur, l, largeur * 1.02]} radius={fin ? largeur * 0.42 : 0.035} smoothness={4}
                    position={[0, l / 2, 0]} material={m.main} castShadow receiveShadow />
        {fin && (
          // Coussinet en caoutchouc au bout du doigt, pour saisir les objets
          <RoundedBox args={[largeur * 0.8, l * 0.5, 0.03]} radius={0.012} position={[0, l * 0.62, largeur * 0.52]} material={m.caoutchouc} />
        )}
        <group position={[0, l, 0]}>{construire(k + 1)}</group>
      </group>
    );
  };
  return <>{construire(0)}</>;
}

/** Angles de pliage maximum de chaque articulation (base, milieu, bout), en degrés : comme une vraie main. */
const PLIAGE_MAX = { doigt: [72, 95, 68], pouce: [45, 55, 50] };

function Robot({ angles }: { angles: MutableRefObject<Angles> }) {
  const m = useMateriaux();
  const base = useRef<THREE.Group>(null);
  const epaule = useRef<THREE.Group>(null);
  const coude = useRef<THREE.Group>(null);
  const poignet = useRef<THREE.Group>(null);
  const doigts = useRef(Object.fromEntries(DOIGTS.map((d) => [d, { current: [] as (THREE.Group | null)[] }])) as
    Record<Doigt, MutableRefObject<(THREE.Group | null)[]>>);

  useFrame(() => {
    const a = angles.current;
    if (base.current) base.current.rotation.y = rad(a.base - 90);
    if (epaule.current) epaule.current.rotation.x = rad(a.epaule - 90);
    if (coude.current) coude.current.rotation.x = rad(180 - a.coude);
    if (poignet.current) poignet.current.rotation.y = rad(a.poignet - 90);
    for (const d of DOIGTS) {
      const f = fermeture(a, d);
      const max = d === "pouce" ? PLIAGE_MAX.pouce : PLIAGE_MAX.doigt;
      doigts.current[d].current.forEach((g, k) => g && (g.rotation.x = rad(max[k] * f)));
    }
  });

  // Doigts : position sur la paume (x), longueurs des 3 phalanges, largeur
  const quatreDoigts: [Doigt, number, number[], number][] = [
    ["index", -0.31, [0.34, 0.25, 0.2], 0.16],
    ["majeur", -0.1, [0.38, 0.27, 0.21], 0.165],
    ["annulaire", 0.11, [0.35, 0.25, 0.2], 0.16],
    ["auriculaire", 0.31, [0.28, 0.2, 0.17], 0.145],
  ];

  return (
    <group>
      {/* Socle fixe */}
      <mesh position={[0, 0.09, 0]} material={m.socle} castShadow receiveShadow>
        <cylinderGeometry args={[1.15, 1.22, 0.18, 64]} />
      </mesh>

      {/* Plateau tournant : la BASE */}
      <group ref={base} position={[0, 0.18, 0]}>
        <mesh position={[0, 0.11, 0]} material={m.aluminium} castShadow receiveShadow>
          <cylinderGeometry args={[0.88, 0.92, 0.22, 64]} />
        </mesh>
        {Array.from({ length: 8 }, (_, i) => (
          <mesh key={i} position={[Math.cos((i / 8) * Math.PI * 2) * 0.72, 0.23, Math.sin((i / 8) * Math.PI * 2) * 0.72]} material={m.acier}>
            <cylinderGeometry args={[0.035, 0.035, 0.03, 12]} />
          </mesh>
        ))}
        {/* Équerres de l'épaule et son servo */}
        {[-1, 1].map((s) => (
          <RoundedBox key={s} args={[0.1, 0.95, 0.62]} radius={0.03} position={[s * 0.45, 0.68, 0]} material={m.aluminium} castShadow receiveShadow />
        ))}
        <Servo m={m} position={[0, 0.62, 0]} rotation-y={Math.PI / 2} />
        <Axe m={m} longueur={1.0} position={[0, 0.98, 0]} />

        {/* Bras : l'ÉPAULE pivote ici */}
        <group ref={epaule} position={[0, 0.98, 0]}>
          {[-1, 1].map((s) => (
            <RoundedBox key={s} args={[0.07, 2.05, 0.3]} radius={0.03} position={[s * 0.33, 1.0, 0]} material={m.aluminium} castShadow receiveShadow />
          ))}
          {[0.55, 1.45].map((y) => (
            <mesh key={y} position={[0, y, 0]} rotation-z={Math.PI / 2} material={m.aluminiumSombre} castShadow>
              <cylinderGeometry args={[0.05, 0.05, 0.6, 16]} />
            </mesh>
          ))}
          <Servo m={m} position={[0, 2.0, 0]} rotation-y={Math.PI / 2} taille={[0.55, 0.38, 0.28]} />
          <Axe m={m} longueur={0.78} position={[0, 2.0, 0]} rayon={0.065} />

          {/* Avant-bras : le COUDE pivote ici */}
          <group ref={coude} position={[0, 2.0, 0]}>
            <mesh position={[0, 0.85, 0]} material={m.aluminium} castShadow receiveShadow>
              <cylinderGeometry args={[0.13, 0.15, 1.6, 40]} />
            </mesh>
            {/* Câble des servos, le long de l'avant-bras */}
            <mesh position={[0.16, 0.85, -0.08]} material={m.servo}>
              <cylinderGeometry args={[0.025, 0.025, 1.5, 10]} />
            </mesh>
            <mesh position={[0, 1.62, 0]} material={m.aluminiumSombre} castShadow>
              <cylinderGeometry args={[0.2, 0.17, 0.14, 40]} />
            </mesh>

            {/* POIGNET : tourne autour de l'axe de l'avant-bras */}
            <group ref={poignet} position={[0, 1.7, 0]}>
              <mesh position={[0, 0.1, 0]} material={m.servo} castShadow>
                <cylinderGeometry args={[0.21, 0.21, 0.2, 40]} />
              </mesh>
              {/* MAIN : la paume fait face à toi (+Z), les doigts montent (+Y) */}
              <group position={[0, 0.2, 0]}>
                <RoundedBox args={[0.88, 0.86, 0.24]} radius={0.08} smoothness={5} position={[0, 0.43, 0]} material={m.main} castShadow receiveShadow />
                <RoundedBox args={[0.6, 0.5, 0.02]} radius={0.01} position={[0, 0.4, -0.125]} material={m.servo} />
                {quatreDoigts.map(([d, x, longueurs, largeur]) => (
                  <group key={d} position={[x, 0.9, 0]} rotation-z={rad(x * -6)}>
                    <Doigt3D m={m} longueurs={longueurs} largeur={largeur} phalanges={doigts.current[d]} />
                  </group>
                ))}
                {/* Pouce : sur le côté de la paume, orienté vers l'extérieur et vers l'avant */}
                <group position={[-0.47, 0.3, 0.06]} rotation={[rad(-10), rad(35), rad(48)]}>
                  <Doigt3D m={m} longueurs={[0.3, 0.24, 0.2]} largeur={0.17} phalanges={doigts.current.pouce} />
                </group>
              </group>
            </group>
          </group>
        </group>
      </group>
    </group>
  );
}

/**
 * Redessiner SEULEMENT quand le robot bouge (la caméra, elle, redemande un dessin toute seule).
 * Sinon la carte graphique travaillerait 60 fois par seconde pour rien, et la détection de la main
 * (qui utilise la même carte graphique) ralentirait : le bras deviendrait saccadé.
 */
function RedessinerSiMouvement({ angles }: { angles: MutableRefObject<Angles> }) {
  const invalidate = useThree((etat) => etat.invalidate);
  useEffect(() => {
    let avant = "";
    let image = 0;
    const verifier = () => {
      const maintenant = Object.values(angles.current).map((v) => v.toFixed(1)).join();
      if (maintenant !== avant) {
        avant = maintenant;
        invalidate();
      }
      image = requestAnimationFrame(verifier);
    };
    image = requestAnimationFrame(verifier);
    return () => cancelAnimationFrame(image);
  }, [angles, invalidate]);
  return null;
}

/** La scène : studio photo (lumières douces, reflets, ombres), et caméra que l'on tourne à la souris. */
export function Bras3D({ angles, fond }: { angles: MutableRefObject<Angles>; fond: string }) {
  return (
    <Canvas frameloop="demand" shadows="soft" dpr={[1, 1.75]} camera={{ position: [6.6, 5.0, 7.8], fov: 36 }} gl={{ antialias: true }}>
      <color attach="background" args={[fond]} />
      <fog attach="fog" args={[fond, 9, 24]} />
      <ambientLight intensity={0.25} />
      <directionalLight position={[5, 9, 6]} intensity={2.4} castShadow shadow-mapSize={[1024, 1024]} shadow-bias={-0.0004}
                        shadow-camera-left={-5} shadow-camera-right={5} shadow-camera-top={7} shadow-camera-bottom={-3} />
      {/* Les reflets du métal : un « studio » de panneaux lumineux, généré ici (aucun fichier à télécharger) */}
      <Environment resolution={256}>
        {/* Un studio clair et chaud : le métal reflète ce qu'il voit, donc il paraît aluminium, pas noir. */}
        <color attach="background" args={["#7d7871"]} />
        <Lightformer intensity={4} position={[0, 6, 2]} scale={[10, 3, 1]} rotation-x={Math.PI / 2} />
        <Lightformer intensity={3} position={[-6, 2, 1]} scale={[2, 8, 1]} rotation-y={Math.PI / 2} />
        <Lightformer intensity={2} position={[6, 3, -2]} scale={[2, 6, 1]} rotation-y={-Math.PI / 2} color="#ffe2c4" />
        <Lightformer intensity={1.5} position={[0, 1.5, -6]} scale={[10, 1.5, 1]} />
        <Lightformer intensity={1} position={[0, 1, 6]} scale={[4, 2, 1]} rotation-y={Math.PI} />
      </Environment>

      <Robot angles={angles} />
      <RedessinerSiMouvement angles={angles} />

      {/* Sol : il reçoit l'ombre douce du robot */}
      <mesh rotation-x={-Math.PI / 2} receiveShadow>
        <planeGeometry args={[60, 60]} />
        <meshStandardMaterial color={fond} roughness={1} />
      </mesh>

      <OrbitControls target={[0, 2.7, 0.6]} enablePan={false} minDistance={5} maxDistance={16} maxPolarAngle={rad(86)} />
    </Canvas>
  );
}
